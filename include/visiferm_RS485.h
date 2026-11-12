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
 * @brief Calibration points of the VisiFerm (standard calibration).
 */
typedef enum
{
    VISIFERM_CAL_ZERO = 0, // CP1: oxygen-free medium (0 %)
    VISIFERM_CAL_AIR       // CP2: air or air-saturated water
} eVisiFermCalPoint;

/**
 * @brief Overall state of a calibration request.
 */
typedef enum
{
    VISIFERM_CAL_IDLE = 0,      // no calibration requested since boot
    VISIFERM_CAL_RUNNING,       // Modbus sequence in progress (DO polling paused)
    VISIFERM_CAL_WAITING_RETRY, // sensor reported "phase not stable", retry in ~110 s (DO polling active)
    VISIFERM_CAL_SUCCESS,       // sensor accepted the calibration
    VISIFERM_CAL_FAILED         // see getCalError() / getCalStatusBits()
} eVisiFermCalState;

/**
 * @brief Reason of a failed calibration.
 */
typedef enum
{
    VISIFERM_CAL_ERR_NONE = 0,
    VISIFERM_CAL_ERR_TIMEOUT,          // no answer from the sensor
    VISIFERM_CAL_ERR_CRC,              // corrupted answer
    VISIFERM_CAL_ERR_BAD_FRAME,        // unexpected answer
    VISIFERM_CAL_ERR_MODBUS_EXCEPTION, // sensor refused the request (see getLastModbusException())
    VISIFERM_CAL_ERR_LOGIN,            // operator level A not accepted (password / word order)
    VISIFERM_CAL_ERR_REJECTED          // sensor ran the calibration and rejected it (see getCalStatusBits())
} eVisiFermCalError;

/**
 * @brief Hamilton VisiFerm RS485 (Arc) Modbus RTU driver.
 *
 * Link to the datasheet:
 *  https://assets-sensors.hamiltoncompany.com/File-Uploads/VisiFerm_RS485-ECS_Manual_EN_10151964-02.pdf?v=1755760046
 *
 * Link to the Programmer's Manual:
 *  https://assets-labs.hamiltoncompany.com/VisiFerm_RS485_Programmers_Manual-111004302-06.pdf?v=1755760545
 *
 * Calibration (chapter 2.7 of the Programmer's Manual):
 *  1. login as operator level A (register 4288), verified by reading it back
 *  2. write the calibration value (float, 0 = automatic) to 5162 (CP1) or 5194 (CP2)
 *  3. read the calibration status 5158 (CP1) or 5190 (CP2), 0 = success
 *  If the sensor answers "phase not stable" (happens on the first try when the sensor's own
 *  measurement interval is > 3 s), the command is sent again once after ~110 s.
 *  The sensor must be stable in the calibration medium for at least 3 minutes before starting.
 *  Each calibration writes to the sensor's non-volatile memory: only trigger it on explicit request.
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

    // ------------------------------------------------------------------ calibration

    /**
     * @brief Request a calibration. Non-blocking: the sequence runs inside update().
     * @param point VISIFERM_CAL_ZERO (CP1) or VISIFERM_CAL_AIR (CP2).
     * @param value 0 = automatic (0 % for CP1, air for CP2), otherwise the oxygen value
     *              in the unit currently configured on the sensor.
     * @return false if a calibration is already in progress.
     */
    bool startCalibration(eVisiFermCalPoint point, float value = 0.0f);

    eVisiFermCalState getCalState() const { return _calState; }
    eVisiFermCalError getCalError() const { return _calError; }
    eVisiFermCalPoint getCalPoint() const { return _calPoint; }

    /** @brief Raw status bits of the last calibration (masked for the requested point, 0 = OK). */
    uint32_t getCalStatusBits() const { return _calStatusBits; }

    /** @brief Oxygen value of the last successful calibration of this point, as stored by the sensor. */
    float getCalStoredValue() const { return _calStoredValue; }

    /** @brief Modbus exception code of the last refused request (0x01..0x04). */
    uint8_t getLastModbusException() const { return _lastException; }

    /** @brief Human readable reason of the first error bit (for logs). */
    static const char *calStatusToText(eVisiFermCalPoint point, uint32_t bits);

    /** @brief Protocol name of a calibration state: IDLE, RUNNING, WAITING_RETRY, SUCCESS, FAILED. */
    static const char *calStateToText(eVisiFermCalState state);

private:
    // polling state
    enum PollState
    {
        POLL_IDLE = 0,
        POLL_WAIT_DO,
        POLL_WAIT_TEMP
    };

    // result of a frame reception attempt
    enum FrameResult
    {
        FRAME_PENDING = 0,
        FRAME_OK,
        FRAME_CRC_ERROR,
        FRAME_EXCEPTION,
        FRAME_BAD
    };

    // calibration sequence steps
    enum CalStep
    {
        CAL_STEP_NONE = 0,
        CAL_STEP_START,
        CAL_STEP_WAIT_LOGIN_ACK,
        CAL_STEP_WAIT_LEVEL,
        CAL_STEP_WAIT_CMD_ACK,
        CAL_STEP_STATUS_DELAY,
        CAL_STEP_WAIT_STATUS,
        CAL_STEP_RETRY_DELAY
    };

    void sendReadDO();
    void sendReadTemp();
    void sendReadRegisters(uint16_t reg, uint16_t count);
    void sendWriteRegisters(uint16_t reg, const uint16_t *words, uint8_t count);
    void sendFrame(const uint8_t *frame, uint8_t len);
    FrameResult tryReadFrame();
    bool parseData(const uint8_t *frame, uint16_t len, float &outValue);
    void cleanSerialBuffer();

    // calibration helpers
    void updateCalibration(uint32_t now);
    FrameResult calWaitFrame(uint32_t now, uint8_t expectedFunc, uint8_t minByteCount);
    void sendLogin();
    void sendCalCommand();
    void finishCalibration(eVisiFermCalState state, eVisiFermCalError error);

    static uint16_t modbusCRC(const uint8_t *buf, uint16_t len);
    static float modbusToFloat(const uint8_t *lswFirst);      // 4 bytes, LSW first, then MSW
    static uint32_t modbusToU32(const uint8_t *lswFirst);     // 4 bytes, LSW first, then MSW
    static void u32ToModbus(uint32_t value, uint16_t *words); // words[0] = LSW, words[1] = MSW
    static void floatToModbus(float value, uint16_t *words);  // words[0] = LSW, words[1] = MSW

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
    static constexpr uint8_t MODBUS_FUNC_READ_HOLDING = 0x03;
    static constexpr uint8_t MODBUS_FUNC_WRITE_MULTIPLE = 0x10;
    static constexpr uint8_t MODBUS_EXCEPTION_FLAG = 0x80;
    static constexpr uint8_t MODBUS_READ_REGISTER_MSG_LEN = 8;
    static constexpr uint8_t MODBUS_WRITE_ACK_LEN = 8; // addr + func + startAddr(2) + qty(2) + CRC(2)
    static constexpr uint8_t MODBUS_EXCEPTION_LEN = 5; // addr + func|0x80 + code + CRC(2)
    static constexpr uint8_t MAX_WRITE_REGS = 4;
    static constexpr uint8_t CRC_LEN = 2;
    static constexpr uint8_t MIN_MSG_LEN = 5; // addr(1) + func(1) + byteCount(1) + CRC(2)
    static constexpr uint8_t DATA_LEN = 20;   // 10 registers * 2 bytes each
    static constexpr uint8_t MAX_BUF_SIZE = 64;

    // calibration constants
    static constexpr uint32_t CAL_RESPONSE_TIMEOUT_MS = 1000;
    static constexpr uint32_t CAL_STATUS_DELAY_MS = 500;   // pause between command and status read
    static constexpr uint32_t CAL_RETRY_DELAY_MS = 110000; // manual: resend after at least 100 s
    static constexpr uint8_t CAL_MAX_RETRIES = 1;

    // operator levels (register 4288): U = 0x03 / pwd 0, A = 0x0C / pwd 18111978, S = 0x30 / pwd 16021966
    static constexpr uint32_t OPERATOR_LEVEL_A = 0x0C;
    static constexpr uint32_t OPERATOR_PASSWORD_A = 18111978UL;

    // calibration status bits (registers 5158 / 5190, CP1 in the low byte, CP2 in the second byte)
    static constexpr uint32_t CAL_MASK_CP1 = 0x000000FF;
    static constexpr uint32_t CAL_MASK_CP2 = 0x0000FF00;
    static constexpr uint32_t CAL_BIT_PHASE_UNSTABLE_CP1 = 0x00000080;
    static constexpr uint32_t CAL_BIT_PHASE_UNSTABLE_CP2 = 0x00008000;

    // receive buffer
    uint8_t _rxBuf[MAX_BUF_SIZE];
    uint16_t _rxLen;
    uint32_t _waitStartMs;
    uint32_t _lastReadTime; // last successful DO/temperature read
    uint32_t _lastPollMs;   // last DO request sent (polling rhythm)
    uint8_t _lastException;

    // calibration state
    CalStep _calStep;
    eVisiFermCalState _calState;
    eVisiFermCalError _calError;
    eVisiFermCalPoint _calPoint;
    float _calValue;
    uint32_t _calStatusBits;
    float _calStoredValue;
    uint32_t _calTimerMs;
    uint8_t _calRetries;

    // VisiFerm registers (manual uses 1-based, Modbus uses 0-based)
    static constexpr uint16_t REG_PMC1 = 2090;    // Primary Measurement Channel 1 (DO), len 10
    static constexpr uint16_t REG_PMC6 = 2410;    // Primary Measurement Channel 6 (Temp), len 10
    static constexpr uint16_t REG_BLOCK_LEN = 10; // number of registers to read

    static constexpr uint16_t REG_OPERATOR_LEVEL = 4288; // len 4: level (u32) + password (u32)
    static constexpr uint16_t REG_CP1_STATUS = 5158;     // len 6: status (u32) + unit (u32) + value (float)
    static constexpr uint16_t REG_CP1_START = 5162;      // len 2: float, 0 = automatic
    static constexpr uint16_t REG_CP2_STATUS = 5190;     // len 6
    static constexpr uint16_t REG_CP2_START = 5194;      // len 2: float, 0 = automatic
    static constexpr uint16_t REG_CAL_STATUS_LEN = 6;
};

#endif // VISIFERM_RS485_H