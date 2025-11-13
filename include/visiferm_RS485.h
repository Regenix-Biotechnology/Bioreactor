#ifndef VISIFERM_RS485_H
#define VISIFERM_RS485_H

#include <Arduino.h>
#include <HardwareSerial.h>

typedef enum
{
    VISIFERM_STATUS_OK = 0,
    VISIFERM_STATUS_NOT_INITIALISED,
    VISIFERM_STATUS_WAITING_RESPONSE,
    VISIFERM_STATUS_TIMEOUT,
    VISIFERM_STATUS_CRC_ERROR,
    VISIFERM_STATUS_BAD_FRAME,
    VISIFERM_STATUS_SENSOR_STATUS_ERROR,

    VISIFERM_STATUS_MAX
} eVisiFermStatus;

/**
 * @brief Hamilton VisiFerm RS485 (Arc) Modbus RTU driver.
 *
 * Link to the datasheet:
 *  https://assets-sensors.hamiltoncompany.com/File-Uploads/VisiFerm_RS485-ECS_Manual_EN_10151964-02.pdf?v=1755760046
 *
 * Link to the Programmer’s Manual:
 *  https://assets-labs.hamiltoncompany.com/VisiFerm_RS485_Programmers_Manual-111004302-06.pdf?v=1755760545
 */
class VisiFermRS485
{
public:
    VisiFermRS485(uint8_t rxPin,
                  uint8_t txPin,
                  HardwareSerial &serial,
                  uint8_t deviceAddress = 1);

    eVisiFermStatus begin();
    eVisiFermStatus update();

    /**
     * @brief Get the latest dissolved oxygen value.
     * @return Oxygen value in the unit defined by the sensor configuration (usually %sat).
     */
    float getOxygen() const { return _oxygen; }

    /**
     * @brief Get the latest temperature value.
     * @return Temperature value in the unit defined by the sensor configuration (usually °C).
     */
    float getTemperature() const { return _temperature; }

    /**
     * @brief Get the timestamp of the last successful read.
     * @return Milliseconds since startup.
     */
    uint32_t getLastReadMs() const { return _lastReadTime; }

    /**
     * @brief Get the current status of the sensor/driver.
     * @return Status enum value.
     */
    eVisiFermStatus getStatus() const { return _status; }

private:
    // polling state
    enum PollState
    {
        POLL_IDLE = 0,
        POLL_WAIT_DO,
        POLL_WAIT_TEMP
    };

    void sendReadDO();
    void sendReadTemp();
    void sendReadRegisters(uint16_t reg, uint16_t count);
    bool tryReadFrame();
    bool parseData(const uint8_t *frame, uint16_t len, float &outValue);
    void cleanSerialBuffer();

    static uint16_t modbusCRC(const uint8_t *buf, uint16_t len);
    static float modbusToFloat(const uint8_t *lswFirst); // 4 bytes, LSW first, then MSW

    HardwareSerial &_serial;
    uint8_t _rxPin;
    uint8_t _txPin;
    uint8_t _addr;

    PollState _pollState;
    eVisiFermStatus _status;

    // latest values
    float _oxygen;      // dissolved oxygen in %sat
    float _temperature; // temperature in °C

    // constants
    static constexpr uint32_t BAUD_RATE = 19200;
    static constexpr uint32_t READ_INTERVAL_MS = 500;
    static constexpr uint32_t RESPONSE_TIMEOUT_MS = 200;
    static constexpr uint16_t MODBUS_FUNC_READ_HOLDING = 0x03;
    static constexpr uint8_t MODBUS_READ_REGISTER_MSG_LEN = 8;
    static constexpr uint8_t CRC_LEN = 2;
    static constexpr uint8_t MIN_MSG_LEN = 5; // addr(1) + func(1) + byteCount(1) + CRC(2)
    static constexpr uint8_t DATA_LEN = 20;   // 10 registers * 2 bytes each
    static constexpr uint8_t MAX_BUF_SIZE = 64;

    // receive buffer
    uint8_t _rxBuf[MAX_BUF_SIZE];
    uint16_t _rxLen;
    uint32_t _waitStartMs;
    uint32_t _lastReadTime;

    // VisiFerm registers (manual uses 1-based, Modbus uses 0-based)
    static constexpr uint16_t REG_PMC1 = 2090;    // Primary Measurement Channel 1 (DO), len 10
    static constexpr uint16_t REG_PMC6 = 2410;    // Primary Measurement Channel 6 (Temp), len 10
    static constexpr uint16_t REG_BLOCK_LEN = 10; // number of registers to read
};

#endif // VISIFERM_RS485_H
