#include "VisiFermRS485.h"

/**
 * Constructor for the VisiFerm RS485 driver.
 */
VisiFermRS485::VisiFermRS485(uint8_t rxPin,
                             uint8_t txPin,
                             HardwareSerial &serial,
                             uint8_t deviceAddress)
    : _serial(serial),
      _rxPin(rxPin),
      _txPin(txPin),
      _addr(deviceAddress),
      _pollState(POLL_IDLE),
      _status(VISIFERM_STATUS_NOT_INITIALISED),
      _rxLen(0),
      _waitStartMs(0),
      _lastReadTime(0),
      _oxygen(NAN),
      _temperature(NAN)
{
}

/**
 * @brief Initialize the VisiFerm RS485 sensor.
 * @return Status of the initialization.
 */
eVisiFermStatus VisiFermRS485::begin()
{
    // VisiFerm: 19200, 8 data, no parity, 2 stop bits.
    _serial.begin(BAUD_RATE, SERIAL_8N2, _rxPin, _txPin);

    _rxLen = 0;
    _pollState = POLL_IDLE;
    _status = VISIFERM_STATUS_NOT_INITIALISED;

    return _status;
}

/**
 * @brief Update the sensor state machine. This needs to be called frequently in the main loop.
 * @return Current status after update.
 */
eVisiFermStatus VisiFermRS485::update()
{
    uint32_t now = millis();

    if (_pollState == POLL_WAIT_DO || _pollState == POLL_WAIT_TEMP)
    {
        if (tryReadFrame())
        {
            // full frame in _rxBuf
            float parsedValue = NAN;

            if (parseData(_rxBuf, _rxLen, parsedValue))
            {
                if (_pollState == POLL_WAIT_DO)
                {
                    _oxygen = parsedValue;
                    sendReadTemp(); // immediately ask for temperature on next update()
                }
                else if (_pollState == POLL_WAIT_TEMP)
                {
                    _temperature = parsedValue;
                    _pollState = POLL_IDLE; // reading cycle complete
                }

                _status = VISIFERM_STATUS_OK;
                _lastReadTime = now;
            }

            _rxLen = 0;
            return _status;
        }

        // timeout
        if (now - _waitStartMs > RESPONSE_TIMEOUT_MS)
        {
            _pollState = POLL_IDLE;
            _rxLen = 0;
            _status = VISIFERM_STATUS_TIMEOUT;
            return _status;
        }

        // still waiting
        return _status;
    }

    if (now - _lastReadTime >= READ_INTERVAL_MS)
    {
        sendReadDO();
        _status = VISIFERM_STATUS_WAITING_RESPONSE;
    }

    return _status;
}

/**
 * @brief Send a Modbus read command for dissolved oxygen (Primary Measurement Channel 1).
 */
void VisiFermRS485::sendReadDO()
{
    cleanSerialBuffer();
    sendReadRegisters(REG_PMC1, REG_BLOCK_LEN);
    _pollState = POLL_WAIT_DO;
    _waitStartMs = millis();
}

/**
 * @brief Send a Modbus read command for temperature (Primary Measurement Channel 6).
 */
void VisiFermRS485::sendReadTemp()
{
    cleanSerialBuffer();
    sendReadRegisters(REG_PMC6, REG_BLOCK_LEN);
    _pollState = POLL_WAIT_TEMP;
    _waitStartMs = millis();
}

/**
 * @brief Send a Modbus read request to the sensor.
 */
void VisiFermRS485::sendReadRegisters(uint16_t reg, uint16_t count)
{
    // Manual addresses start at 1. Modbus frame needs startAddr-1.
    uint16_t startAddr = reg - 1;

    uint8_t frame[MODBUS_READ_REGISTER_MSG_LEN];
    frame[0] = _addr;                                                        // device address
    frame[1] = MODBUS_FUNC_READ_HOLDING;                                     // function code 0x03
    frame[2] = (startAddr >> 8) & 0xFF;                                      // start address high
    frame[3] = (startAddr) & 0xFF;                                           // start address low
    frame[4] = (count >> 8) & 0xFF;                                          // quantity high
    frame[5] = (count) & 0xFF;                                               // quantity low
    uint16_t crc = modbusCRC(frame, MODBUS_READ_REGISTER_MSG_LEN - CRC_LEN); // CRC over first 6 bytes
    frame[6] = crc & 0xFF;                                                   // CRC low
    frame[7] = (crc >> 8) & 0xFF;                                            // CRC high

    // drive RS485 TX
    _serial.write(frame, MODBUS_READ_REGISTER_MSG_LEN);
    _serial.flush();

    // prepare RX
    _rxLen = 0;
}

/**
 * @brief Read available bytes from serial and check if a full Modbus frame has been received.
 * @return True if a full frame has been received (valid or invalid), false if still waiting.
 */
bool VisiFermRS485::tryReadFrame()
{
    while (_serial.available() && _rxLen < sizeof(_rxBuf))
    {
        _rxBuf[_rxLen++] = _serial.read();
    }

    if (_rxLen < MIN_MSG_LEN)
        return false;

    // fixed structure:
    // [0]=addr, [1]=func, [2]=byteCount (should be 0x14 = 20), [3..] data, [end-2..end-1]=crc
    uint8_t byteCount = _rxBuf[2];
    uint16_t expectedLen = byteCount + MIN_MSG_LEN; // addr+func+byteCount + data + CRC
    if (_rxLen < expectedLen)
        return false;

    // check CRC
    uint16_t frameCRC = (_rxBuf[expectedLen - 1] << 8) | _rxBuf[expectedLen - CRC_LEN];
    uint16_t calcCRC = modbusCRC(_rxBuf, expectedLen - CRC_LEN);
    if (frameCRC != calcCRC)
    {
        _status = VISIFERM_STATUS_CRC_ERROR;
        return true; // we have a frame, but bad
    }

    return true;
}

/**
 * @brief Parsing logic for Primary Measurement Channel frames.
 * @return True if parsing was successful, false on error.
 */
bool VisiFermRS485::parseData(const uint8_t *frame, uint16_t len, float &outValue)
{
    if (len < MIN_MSG_LEN)
    {
        _status = VISIFERM_STATUS_BAD_FRAME;
        return false;
    }

    if (frame[1] != MODBUS_FUNC_READ_HOLDING)
    {
        _status = VISIFERM_STATUS_BAD_FRAME;
        return false;
    }

    uint8_t byteCount = frame[2];
    if (byteCount < DATA_LEN || len < (uint16_t)(byteCount + MIN_MSG_LEN))
    {
        _status = VISIFERM_STATUS_BAD_FRAME;
        return false;
    }

    const uint8_t *data = &frame[3]; // data block (expected 20 bytes)

    // Reg3/Reg4 = value (float, LSW first)
    outValue = modbusToFloat(&data[4]);

    // Reg5/Reg6 = measurement status
    uint16_t statusLo = (uint16_t)data[8] << 8 | data[9];
    uint16_t statusHi = (uint16_t)data[10] << 8 | data[11];
    if (statusLo != 0 || statusHi != 0)
    {
        // sensor reports warning/error in measurement
        _status = VISIFERM_STATUS_SENSOR_STATUS_ERROR;
        return false;
    }

    return true;
}

/**
 * @brief Calculate Modbus CRC16 for a data buffer.
 * @return CRC16 value.
 */
uint16_t VisiFermRS485::modbusCRC(const uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0xFFFF;

    for (uint16_t pos = 0; pos < len; pos++)
    {
        crc ^= (uint16_t)buf[pos];

        for (uint8_t i = 0; i < 8; i++)
        {
            if (crc & 0x0001)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}

/**
 * @brief Convert 4 bytes from Modbus format (LSW first) to float.
 */
float VisiFermRS485::modbusToFloat(const uint8_t *lswFirst)
{
    // data order from sensor for a float:
    // Reg3 low, Reg3 high, Reg4 low, Reg4 high
    // -> word order: LSW, MSW
    uint32_t lsw = ((uint32_t)lswFirst[0] << 8) | lswFirst[1];
    uint32_t msw = ((uint32_t)lswFirst[2] << 8) | lswFirst[3];
    uint32_t raw = (msw << 16) | lsw;
    float f;
    memcpy(&f, &raw, sizeof(f));
    return f;
}

/**
 * @brief Clear any pending data in the serial buffer.
 */
void VisiFermRS485::cleanSerialBuffer()
{
    if (_serial.available())
    {
        while (_serial.available())
        {
            _serial.read();
        }
    }
}
