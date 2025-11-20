#include "AtlasBase.h"

/**
 * @brief Initialize the Atlas device.
 * @return eAtlasStatus Status of the initialization operation.
 */
eAtlasStatus AtlasBase::begin()
{
    _pWire->begin();
    return _status = ATLAS_STATUS_INITIALIZED;
}

/**
 * @brief Update the Atlas device state, handling measurement requests and polling.
 * This function should be called periodically in the main loop.
 */
void AtlasBase::update()
{
    unsigned long now = millis();

    if ((now - _lastCommTime) > COMM_LOSS_TIMEOUT_MS && _lastCommTime != 0)
    {
        _lastValue = 0.0;
    }

    requestMeasurement();

    if (_state == ST_WAITING && now >= _nextPollDue)
    {
        if (now - _cmdSentAt > NB_MAX_CONVERSION_MS)
        {
            _state = ST_ERROR;
            _status = ATLAS_STATUS_TIMEOUT_EXCEEDED;
        }
        else
        {
            pollOnce();
        }
    }
}

/**
 * @brief Send a measurement request to the Atlas device.
 * @return bool True if the request was sent successfully, false otherwise.
 *
 * @note The logic is designed to ask a measurement to the sensor, then wait in a non-blocking way
 * for the sensor to be ready before polling for the result.
 */
bool AtlasBase::requestMeasurement()
{
    if (_state == ST_WAITING)
        return false;

    if (writeI2C(MEASUREMENT_REQUEST_CMD, strlen(MEASUREMENT_REQUEST_CMD), true) != ATLAS_STATUS_OK)
    {
        _state = ST_ERROR;
        _status = ATLAS_STATUS_FAILED_TO_SEND_REQUEST;
        return false;
    }
    _state = ST_WAITING;
    _cmdSentAt = millis();
    _nextPollDue = millis();
    return true;
}

/**
 * @brief Read the measurement result from the Atlas device and process it.
 * @return eAtlasStatus Status of the polling operation.
 */
eAtlasStatus AtlasBase::pollOnce()
{
    char buf[RESPONSE_BUFFER_SIZE];
    memset(buf, 0, RESPONSE_BUFFER_SIZE);

    _pWire->requestFrom(_i2cAddress, RESPONSE_BUFFER_SIZE);

    if (!_pWire->available())
    {
        _nextPollDue = millis() + NB_POLL_INTERVAL_MS;
        return _status;
    }

    uint8_t statusByte = _pWire->read();
    size_t i = 0;
    while (_pWire->available() && i < RESPONSE_BUFFER_SIZE - 1)
    {
        buf[i++] = (char)_pWire->read();
    }
    buf[i] = '\0';

    if (statusByte == SUCCESS_STATUS_BYTE)
    {
        float val = cleanString(buf);
        if (val == 0.0f || isValueFault(val))
        {
            _lastValue = 0.0;
            _state = ST_ERROR;
            return _status = ATLAS_STATUS_PARSING_ERROR;
        }
        _lastValue = val;
        _lastCommTime = millis();
        _lastReadyTime = millis();
        _state = ST_IDLE;
        return _status = ATLAS_STATUS_OK;
    }
    else if (statusByte == PENDING_STATUS_BYTE || statusByte == FAILED_STATUS_BYTE)
    {
        _nextPollDue = millis() + NB_POLL_INTERVAL_MS;
        return _status;
    }
    else
    {
        _state = ST_ERROR;
        return _status = ATLAS_STATUS_DEVICE_ERROR;
    }
}

/**
 * @brief Get the age of the last successful measurement.
 * @return unsigned long Age in milliseconds.
 */
unsigned long AtlasBase::getAgeMs() const
{
    return _lastReadyTime == 0 ? (unsigned long)0xFFFFFFFF : millis() - _lastReadyTime;
}

/**
 * @brief Clear the I2C buffer of the Atlas device.
 */
void AtlasBase::ClearI2CBuffer()
{
    delay(NB_MAX_CONVERSION_MS);
    _pWire->requestFrom(_i2cAddress, RESPONSE_BUFFER_SIZE);
    while (_pWire->available())
        _pWire->read();
}

/**
 * @brief Clean a response string from the Atlas device, extracting the numeric value.
 * @param buf The raw response string from the device.
 * @return float The cleaned number containing the numeric value.
 */
float AtlasBase::cleanString(const char *buf)
{
    String s(buf);
    s.trim();
    while (s.length() > 0 && !(isDigit(s[0]) || s[0] == '-' || s[0] == '.'))
    {
        s = s.substring(1);
    }
    return s.toFloat();
}

/**
 * @brief Write a command to the Atlas device over I2C.
 * @param payload The command payload to send.
 * @param len The length of the payload.
 * @param nullTerminate Whether to append a null terminator to the command. Default is true.
 * @return eAtlasStatus Status of the write operation.
 */
eAtlasStatus AtlasBase::writeI2C(const char *payload, size_t len, bool nullTerminate)
{
    _pWire->beginTransmission(_i2cAddress);

    for (size_t i = 0; i < len; ++i)
        _pWire->write((uint8_t)payload[i]);

    if (nullTerminate)
        _pWire->write((uint8_t)0x00);

    if (_pWire->endTransmission() != 0)
        return _status = ATLAS_STATUS_FAILED_TO_SEND_REQUEST;

    return _status = ATLAS_STATUS_OK;
}