#ifndef ATLAS_BASE_H
#define ATLAS_BASE_H

#include <Arduino.h>
#include <Wire.h>

typedef enum
{
    ATLAS_STATUS_OK = 0,
    ATLAS_STATUS_NOT_INITIALISED,
    ATLAS_STATUS_INITIALIZED,
    ATLAS_STATUS_FAILED_TO_SEND_REQUEST,
    ATLAS_STATUS_TIMEOUT_EXCEEDED,
    ATLAS_STATUS_PARSING_ERROR,
    ATLAS_STATUS_DEVICE_ERROR,
    eAtlasStatus_MAX
} eAtlasStatus;

typedef enum
{
    CAL_PH_4 = 0,
    CAL_PH_7,
    CAL_PH_10,
    CAL_TEMP_100C,
} eCalibrationValues;

/**
 * @brief Base class for Atlas Scientific EZO circuit drivers (I2C, ASCII protocol)
 * This class provides common functionality for communicating with Atlas EZO devices over I2C.
 * It handles the I2C communication and provides a simple interface for sending commands and receiving responses.
 */
class AtlasBase
{
public:
    AtlasBase(TwoWire *pWire, uint8_t i2cAddress)
        : _pWire(pWire), _i2cAddress(i2cAddress) {}

    virtual ~AtlasBase() {}

    eAtlasStatus begin();
    void update();
    float getLastValue() const { return _lastValue; }
    unsigned long getAgeMs() const;
    eAtlasStatus getStatus() const { return _status; }
    virtual eAtlasStatus calibrateSinglePoint(eCalibrationValues value) = 0;

protected:
    virtual bool isValueFault(float v) const { return false; };
    float cleanString(const char *buf);
    eAtlasStatus writeI2C(const char *payload, size_t len, bool nullTerminate = true);
    bool requestMeasurement();
    eAtlasStatus pollOnce();
    void ClearI2CBuffer();

protected:
    enum InternalState : uint8_t
    {
        ST_IDLE = 0,
        ST_WAITING,
        ST_ERROR
    };
    InternalState _state = ST_IDLE;

    TwoWire *_pWire;
    const uint8_t _i2cAddress;

    unsigned long _cmdSentAt = 0;
    unsigned long _lastReadyTime = 0;
    unsigned long _nextPollDue = 0;
    unsigned long _lastCommTime = 0;
    float _lastValue = 0.0;
    eAtlasStatus _status = ATLAS_STATUS_NOT_INITIALISED;

    static constexpr uint8_t SUCCESS_STATUS_BYTE = 0x01;
    static constexpr uint8_t FAILED_STATUS_BYTE = 0x02;
    static constexpr uint8_t PENDING_STATUS_BYTE = 0xFE;
    static constexpr size_t RESPONSE_BUFFER_SIZE = 64;
    static constexpr const char *MEASUREMENT_REQUEST_CMD = "R";
    static constexpr unsigned long NB_POLL_INTERVAL_MS = 50;
    static constexpr unsigned long NB_MAX_CONVERSION_MS = 900;
    static constexpr unsigned long COMM_LOSS_TIMEOUT_MS = 15000;
    static constexpr float EPSILON = 0.05f; // General small value for float comparisons
};

#endif // ATLAS_BASE_H
