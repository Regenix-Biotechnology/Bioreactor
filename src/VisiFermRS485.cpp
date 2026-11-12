#include "visiferm_RS485.h"

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
      _oxygen(0.0),
      _temperature(0.0),
      _rxLen(0),
      _waitStartMs(0),
      _lastReadTime(0),
      _lastPollMs(0),
      _lastException(0),
      _calStep(CAL_STEP_NONE),
      _calState(VISIFERM_CAL_IDLE),
      _calError(VISIFERM_CAL_ERR_NONE),
      _calPoint(VISIFERM_CAL_AIR),
      _calValue(0.0f),
      _calStatusBits(0),
      _calStoredValue(0.0f),
      _calTimerMs(0),
      _calRetries(0)
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

    // Calibration has priority over DO polling, but never interrupts a read in progress.
    // While waiting for a retry (~110 s), normal polling continues.
    if (_calStep != CAL_STEP_NONE && _pollState == POLL_IDLE)
    {
        bool retryWaiting = (_calStep == CAL_STEP_RETRY_DELAY) && (now - _calTimerMs < CAL_RETRY_DELAY_MS);
        if (!retryWaiting)
        {
            updateCalibration(now);
            return _status;
        }
    }

    if (_pollState == POLL_WAIT_DO || _pollState == POLL_WAIT_TEMP)
    {
        FrameResult result = tryReadFrame();

        if (result == FRAME_PENDING)
        {
            if (now - _waitStartMs > RESPONSE_TIMEOUT_MS)
            {
                _pollState = POLL_IDLE;
                _rxLen = 0;
                _status = VISIFERM_STATUS_TIMEOUT;
            }
            return _status; // still waiting or timed out
        }

        if (result == FRAME_OK)
        {
            float parsedValue = 0.0;

            if (parseData(_rxBuf, _rxLen, parsedValue))
            {
                if (_pollState == POLL_WAIT_DO)
                {
                    _oxygen = parsedValue;
                    _status = VISIFERM_STATUS_OK;
                    _rxLen = 0;
                    sendReadTemp(); // immediately ask for temperature
                    return _status;
                }

                _temperature = parsedValue;
                _status = VISIFERM_STATUS_OK;
                _lastReadTime = now;
            }
            // parseData() sets _status on failure
        }
        else if (result == FRAME_CRC_ERROR)
        {
            _status = VISIFERM_STATUS_CRC_ERROR;
        }
        else
        {
            _status = VISIFERM_STATUS_BAD_FRAME;
        }

        // reading cycle complete (success or failure)
        _pollState = POLL_IDLE;
        _rxLen = 0;
        return _status;
    }

    if (now - _lastPollMs >= READ_INTERVAL_MS)
    {
        _lastPollMs = now;
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
    sendReadRegisters(REG_PMC1, REG_BLOCK_LEN);
    _pollState = POLL_WAIT_DO;
    _waitStartMs = millis();
}

/**
 * @brief Send a Modbus read command for temperature (Primary Measurement Channel 6).
 */
void VisiFermRS485::sendReadTemp()
{
    sendReadRegisters(REG_PMC6, REG_BLOCK_LEN);
    _pollState = POLL_WAIT_TEMP;
    _waitStartMs = millis();
}

/**
 * @brief Send a Modbus read request (function 0x03) to the sensor.
 */
void VisiFermRS485::sendReadRegisters(uint16_t reg, uint16_t count)
{
    // Manual addresses start at 1. Modbus frame needs startAddr-1.
    uint16_t startAddr = reg - 1;

    uint8_t frame[MODBUS_READ_REGISTER_MSG_LEN];
    frame[0] = _addr;                    // device address
    frame[1] = MODBUS_FUNC_READ_HOLDING; // function code 0x03
    frame[2] = (startAddr >> 8) & 0xFF;  // start address high
    frame[3] = (startAddr) & 0xFF;       // start address low
    frame[4] = (count >> 8) & 0xFF;      // quantity high
    frame[5] = (count) & 0xFF;           // quantity low
    uint16_t crc = modbusCRC(frame, MODBUS_READ_REGISTER_MSG_LEN - CRC_LEN);
    frame[6] = crc & 0xFF;        // CRC low
    frame[7] = (crc >> 8) & 0xFF; // CRC high

    sendFrame(frame, MODBUS_READ_REGISTER_MSG_LEN);
}

/**
 * @brief Send a Modbus write multiple registers request (function 0x10) to the sensor.
 * @param words register values, each sent high byte first.
 */
void VisiFermRS485::sendWriteRegisters(uint16_t reg, const uint16_t *words, uint8_t count)
{
    if (count == 0 || count > MAX_WRITE_REGS)
        return;

    uint16_t startAddr = reg - 1;

    uint8_t frame[7 + 2 * MAX_WRITE_REGS + CRC_LEN];
    uint8_t n = 0;
    frame[n++] = _addr;
    frame[n++] = MODBUS_FUNC_WRITE_MULTIPLE;
    frame[n++] = (startAddr >> 8) & 0xFF;
    frame[n++] = (startAddr) & 0xFF;
    frame[n++] = 0x00;      // quantity high
    frame[n++] = count;     // quantity low
    frame[n++] = count * 2; // byte count
    for (uint8_t i = 0; i < count; i++)
    {
        frame[n++] = (words[i] >> 8) & 0xFF;
        frame[n++] = (words[i]) & 0xFF;
    }
    uint16_t crc = modbusCRC(frame, n);
    frame[n++] = crc & 0xFF;
    frame[n++] = (crc >> 8) & 0xFF;

    sendFrame(frame, n);
}

/**
 * @brief Flush stale RX bytes, send a frame and prepare reception.
 */
void VisiFermRS485::sendFrame(const uint8_t *frame, uint8_t len)
{
    cleanSerialBuffer();

    // drive RS485 TX
    _serial.write(frame, len);
    _serial.flush();

    // prepare RX
    _rxLen = 0;
}

/**
 * @brief Read available bytes from serial and check if a full Modbus frame has been received.
 * Handles read answers (0x03), write acknowledgements (0x10) and exception answers (func | 0x80).
 * @return FRAME_PENDING while incomplete, otherwise the result of the frame check.
 */
VisiFermRS485::FrameResult VisiFermRS485::tryReadFrame()
{
    while (_serial.available() && _rxLen < sizeof(_rxBuf))
    {
        _rxBuf[_rxLen++] = _serial.read();
    }

    if (_rxLen < 2)
        return FRAME_PENDING;

    uint8_t func = _rxBuf[1];
    uint16_t expectedLen;

    if (func & MODBUS_EXCEPTION_FLAG)
    {
        expectedLen = MODBUS_EXCEPTION_LEN;
    }
    else if (func == MODBUS_FUNC_WRITE_MULTIPLE)
    {
        expectedLen = MODBUS_WRITE_ACK_LEN;
    }
    else if (func == MODBUS_FUNC_READ_HOLDING)
    {
        if (_rxLen < 3)
            return FRAME_PENDING;
        // [0]=addr, [1]=func, [2]=byteCount, [3..] data, [end-2..end-1]=crc
        expectedLen = _rxBuf[2] + MIN_MSG_LEN;
    }
    else
    {
        return FRAME_BAD;
    }

    if (expectedLen > sizeof(_rxBuf))
        return FRAME_BAD;

    if (_rxLen < expectedLen)
        return FRAME_PENDING;

    // check CRC
    uint16_t frameCRC = (_rxBuf[expectedLen - 1] << 8) | _rxBuf[expectedLen - CRC_LEN];
    uint16_t calcCRC = modbusCRC(_rxBuf, expectedLen - CRC_LEN);
    if (frameCRC != calcCRC)
        return FRAME_CRC_ERROR;

    if (_rxBuf[0] != _addr)
        return FRAME_BAD;

    if (func & MODBUS_EXCEPTION_FLAG)
    {
        _lastException = _rxBuf[2];
        return FRAME_EXCEPTION;
    }

    return FRAME_OK;
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

// ============================================================================ calibration

/**
 * @brief Request a calibration. The sequence is executed by update().
 */
bool VisiFermRS485::startCalibration(eVisiFermCalPoint point, float value)
{
    if (_calStep != CAL_STEP_NONE)
        return false; // already running

    _calPoint = point;
    _calValue = value;
    _calRetries = 0;
    _calStatusBits = 0;
    _calError = VISIFERM_CAL_ERR_NONE;
    _calState = VISIFERM_CAL_RUNNING;
    _calStep = CAL_STEP_START;
    return true;
}

/**
 * @brief Calibration state machine, called by update() when the bus is free.
 */
void VisiFermRS485::updateCalibration(uint32_t now)
{
    switch (_calStep)
    {
    case CAL_STEP_START:
    case CAL_STEP_RETRY_DELAY: // retry delay elapsed (checked in update())
        _calState = VISIFERM_CAL_RUNNING;
        sendLogin();
        _calStep = CAL_STEP_WAIT_LOGIN_ACK;
        break;

    case CAL_STEP_WAIT_LOGIN_ACK:
        if (calWaitFrame(now, MODBUS_FUNC_WRITE_MULTIPLE, 0) != FRAME_OK)
            break;
        // read the operator level back to check the login
        sendReadRegisters(REG_OPERATOR_LEVEL, 4);
        _waitStartMs = now;
        _calStep = CAL_STEP_WAIT_LEVEL;
        break;

    case CAL_STEP_WAIT_LEVEL:
    {
        if (calWaitFrame(now, MODBUS_FUNC_READ_HOLDING, 8) != FRAME_OK)
            break;
        uint32_t level = modbusToU32(&_rxBuf[3]); // Reg1/Reg2
        if (level != OPERATOR_LEVEL_A)
        {
            finishCalibration(VISIFERM_CAL_FAILED, VISIFERM_CAL_ERR_LOGIN);
            break;
        }
        sendCalCommand();
        _calStep = CAL_STEP_WAIT_CMD_ACK;
        break;
    }

    case CAL_STEP_WAIT_CMD_ACK:
        if (calWaitFrame(now, MODBUS_FUNC_WRITE_MULTIPLE, 0) != FRAME_OK)
            break;
        _calTimerMs = now;
        _calStep = CAL_STEP_STATUS_DELAY;
        break;

    case CAL_STEP_STATUS_DELAY:
        if (now - _calTimerMs < CAL_STATUS_DELAY_MS)
            break;
        sendReadRegisters(_calPoint == VISIFERM_CAL_ZERO ? REG_CP1_STATUS : REG_CP2_STATUS,
                          REG_CAL_STATUS_LEN);
        _waitStartMs = now;
        _calStep = CAL_STEP_WAIT_STATUS;
        break;

    case CAL_STEP_WAIT_STATUS:
    {
        if (calWaitFrame(now, MODBUS_FUNC_READ_HOLDING, REG_CAL_STATUS_LEN * 2) != FRAME_OK)
            break;

        const uint8_t *data = &_rxBuf[3];
        uint32_t status = modbusToU32(&data[0]); // Reg1/Reg2: status bits
        // Reg3/Reg4: unit of the last successful calibration (not used)
        _calStoredValue = modbusToFloat(&data[8]); // Reg5/Reg6: value of the last successful calibration

        uint32_t mask = (_calPoint == VISIFERM_CAL_ZERO) ? CAL_MASK_CP1 : CAL_MASK_CP2;
        uint32_t phaseUnstable = (_calPoint == VISIFERM_CAL_ZERO) ? CAL_BIT_PHASE_UNSTABLE_CP1
                                                                  : CAL_BIT_PHASE_UNSTABLE_CP2;
        _calStatusBits = status & mask;

        if (_calStatusBits == 0)
        {
            finishCalibration(VISIFERM_CAL_SUCCESS, VISIFERM_CAL_ERR_NONE);
        }
        else if ((_calStatusBits & phaseUnstable) && _calRetries < CAL_MAX_RETRIES)
        {
            // Expected on the first try if the sensor measurement interval is > 3 s:
            // the sensor switches to 3 s and needs 100 s of history before checking stability.
            _calRetries++;
            _calTimerMs = now;
            _calStep = CAL_STEP_RETRY_DELAY;
            _calState = VISIFERM_CAL_WAITING_RETRY;
        }
        else
        {
            finishCalibration(VISIFERM_CAL_FAILED, VISIFERM_CAL_ERR_REJECTED);
        }
        break;
    }

    default:
        finishCalibration(VISIFERM_CAL_FAILED, VISIFERM_CAL_ERR_BAD_FRAME);
        break;
    }
}

/**
 * @brief Wait for the answer of a calibration request. Ends the calibration on error.
 * @param expectedFunc expected Modbus function code of the answer.
 * @param minByteCount for read answers, minimum number of data bytes.
 * @return FRAME_PENDING while waiting, FRAME_OK when a valid answer is in _rxBuf, other values on error.
 */
VisiFermRS485::FrameResult VisiFermRS485::calWaitFrame(uint32_t now, uint8_t expectedFunc, uint8_t minByteCount)
{
    FrameResult result = tryReadFrame();

    if (result == FRAME_PENDING)
    {
        if (now - _waitStartMs > CAL_RESPONSE_TIMEOUT_MS)
        {
            _rxLen = 0;
            finishCalibration(VISIFERM_CAL_FAILED, VISIFERM_CAL_ERR_TIMEOUT);
            return FRAME_BAD;
        }
        return FRAME_PENDING;
    }

    if (result == FRAME_OK)
    {
        bool funcOk = (_rxBuf[1] == expectedFunc);
        bool lenOk = (expectedFunc != MODBUS_FUNC_READ_HOLDING) || (_rxBuf[2] >= minByteCount);
        if (funcOk && lenOk)
            return FRAME_OK;
        result = FRAME_BAD;
    }

    _rxLen = 0;
    if (result == FRAME_CRC_ERROR)
        finishCalibration(VISIFERM_CAL_FAILED, VISIFERM_CAL_ERR_CRC);
    else if (result == FRAME_EXCEPTION)
        finishCalibration(VISIFERM_CAL_FAILED, VISIFERM_CAL_ERR_MODBUS_EXCEPTION);
    else
        finishCalibration(VISIFERM_CAL_FAILED, VISIFERM_CAL_ERR_BAD_FRAME);
    return result;
}

/**
 * @brief Switch the sensor to operator level A (register 4288: level u32 + password u32).
 * The level falls back to U on sensor power-up.
 */
void VisiFermRS485::sendLogin()
{
    uint16_t words[4];
    u32ToModbus(OPERATOR_LEVEL_A, &words[0]);
    u32ToModbus(OPERATOR_PASSWORD_A, &words[2]);
    sendWriteRegisters(REG_OPERATOR_LEVEL, words, 4);
    _waitStartMs = millis();
}

/**
 * @brief Write the calibration value to the CP1 or CP2 start register.
 */
void VisiFermRS485::sendCalCommand()
{
    uint16_t words[2];
    floatToModbus(_calValue, words);
    sendWriteRegisters(_calPoint == VISIFERM_CAL_ZERO ? REG_CP1_START : REG_CP2_START, words, 2);
    _waitStartMs = millis();
}

void VisiFermRS485::finishCalibration(eVisiFermCalState state, eVisiFermCalError error)
{
    _calState = state;
    _calError = error;
    _calStep = CAL_STEP_NONE;
    _lastPollMs = millis() - READ_INTERVAL_MS; // resume DO polling right away
}

/**
 * @brief Text of the first error bit of a calibration status (registers 5158 / 5190).
 */
const char *VisiFermRS485::calStatusToText(eVisiFermCalPoint point, uint32_t bits)
{
    static const char *const texts[8] = {
        "oxygen value too low",
        "oxygen value too high",
        "temperature too low",
        "temperature too high",
        "temperature not stable",
        "phase too low",
        "phase too high",
        "phase not stable"};

    uint8_t b = (point == VISIFERM_CAL_ZERO) ? (bits & 0xFF) : ((bits >> 8) & 0xFF);
    if (b == 0)
        return "ok";
    for (uint8_t i = 0; i < 8; i++)
    {
        if (b & (1 << i))
            return texts[i];
    }
    return "unknown";
}

/**
 * @brief Protocol name of a calibration state (used in SENSORS and STATUS lines).
 */
const char *VisiFermRS485::calStateToText(eVisiFermCalState state)
{
    switch (state)
    {
    case VISIFERM_CAL_IDLE:
        return "IDLE";
    case VISIFERM_CAL_RUNNING:
        return "RUNNING";
    case VISIFERM_CAL_WAITING_RETRY:
        return "WAITING_RETRY";
    case VISIFERM_CAL_SUCCESS:
        return "SUCCESS";
    case VISIFERM_CAL_FAILED:
        return "FAILED";
    default:
        return "UNKNOWN";
    }
}

// ============================================================================ helpers

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
 * @brief Convert 4 bytes from Modbus format (LSW first) to a 32-bit value.
 */
uint32_t VisiFermRS485::modbusToU32(const uint8_t *lswFirst)
{
    // Reg low word (high byte, low byte), then high word (high byte, low byte)
    uint32_t lsw = ((uint32_t)lswFirst[0] << 8) | lswFirst[1];
    uint32_t msw = ((uint32_t)lswFirst[2] << 8) | lswFirst[3];
    return (msw << 16) | lsw;
}

/**
 * @brief Convert 4 bytes from Modbus format (LSW first) to float.
 */
float VisiFermRS485::modbusToFloat(const uint8_t *lswFirst)
{
    uint32_t raw = modbusToU32(lswFirst);
    float f;
    memcpy(&f, &raw, sizeof(f));
    return f;
}

/**
 * @brief Split a 32-bit value into two Modbus registers, LSW first (same order as the readings).
 */
void VisiFermRS485::u32ToModbus(uint32_t value, uint16_t *words)
{
    words[0] = value & 0xFFFF;         // LSW
    words[1] = (value >> 16) & 0xFFFF; // MSW
}

/**
 * @brief Split a float into two Modbus registers, LSW first (same order as the readings).
 */
void VisiFermRS485::floatToModbus(float value, uint16_t *words)
{
    uint32_t raw;
    memcpy(&raw, &value, sizeof(raw));
    u32ToModbus(raw, words);
}

/**
 * @brief Clear any pending data in the serial buffer.
 */
void VisiFermRS485::cleanSerialBuffer()
{
    while (_serial.available())
    {
        _serial.read();
    }
}