#include "O2Sensor.h"

/**
 * @brief Initializes the O2 sensor
 * @param pWire Pointer to the I2C interface (default: Wire)
 */
O2Sensor::O2Sensor(TwoWire *pWire)
    : _pWire(pWire), status(O2_SENSOR_STATUS_NOT_INITIALIZED) {}

/**
 * @brief Initializes I2C communication and checks sensor availability
 * @return Initialization success state
 */
eO2SensorStatus O2Sensor::begin()
{
    _pWire->begin();
    _pWire->beginTransmission(I2C_ADDRESS);
    if (_pWire->endTransmission() != 0)
        return this->status = O2_SENSOR_STATUS_NOT_INITIALIZED;

    return this->status = O2_SENSOR_STATUS_INITIALIZED;
}

/**
 * @brief Get oxygen concentration in volume percentage
 * @return Oxygen concentration (% Vol)
 */
float O2Sensor::getO2()
{
    float vol;
    uint8_t buf[DATA_BUFFER_SIZE] = {};
    readData(OXYGEN_DATA, buf, DATA_BUFFER_SIZE);
    vol = static_cast<float>(buf[0]) +
          static_cast<float>(buf[1]) / DECIMAL_PLACE_1 +
          static_cast<float>(buf[2]) / DECIMAL_PLACE_2;
    return vol;
}

/**
 * @brief Check calibration status
 * @return Calibration state (0: no calibration, 1: 20.9% calibrated, 2: 99.5% calibrated, 3: both calibrated)
 * @details The calibration state is a bitwise representation of the calibration status.
 */
uint8_t O2Sensor::readCalibrationState()
{
    uint8_t data = 0;
    readData(CALIBRATION_STATE, &data, 1);
    return data;
}

/**
 * @brief Calibrates the sensor with an O2 concentration of 20.9% Vol (ambient air)
 * @return True if calibration is successful, false otherwise
 *
 * @warning This function will block for 2 seconds while the calibration is performed.
 */
bool O2Sensor::calibration_20_9()
{
    uint8_t state = 0, data = CALIBRATION_20_9;
    writeData(CALIBRATION_SENSOR, &data, 1);
    delay(CALIBRATION_DELAY);
    state = readCalibrationState();
    return (state & CALIBRATION_20_9);
}

/**
 * @brief Calibrates the sensor with an O2 concentration of 99.5% Vol
 * @return True if calibration is successful, false otherwise
 *
 * @warning This function will block for 2 seconds while the calibration is performed.
 */
bool O2Sensor::calibration_99_5()
{
    uint8_t state = 0, data = CALIBRATION_99_5;
    writeData(CALIBRATION_SENSOR, &data, 1);
    delay(CALIBRATION_DELAY);
    state = readCalibrationState();
    return (state & CALIBRATION_99_5);
}

/**
 * @brief Clears calibration data
 * @return True if successful, false otherwise
 *
 * @warning This function will block for 2 seconds while the calibration is performed.
 */
bool O2Sensor::clearCalibration()
{
    uint8_t state = 0, data = CALIBRATION_CLEAR;
    writeData(CALIBRATION_SENSOR, &data, 1);
    delay(CALIBRATION_DELAY);
    state = readCalibrationState();
    return (state == 0);
}

/**
 * @brief Writes data to the sensor
 * @param reg Register address
 * @param data Data to write
 * @param len Number of bytes to write
 * @return Status of the write operation
 */
eO2SensorStatus O2Sensor::writeData(uint8_t reg, uint8_t *data, uint8_t len)
{
    _pWire->beginTransmission(I2C_ADDRESS);
    _pWire->write(reg);
    for (uint8_t i = 0; i < len; i++)
        _pWire->write(data[i]);

    if (_pWire->endTransmission() != 0)
        return this->status = O2_SENSOR_STATUS_FAILED_TO_SEND_REQUEST;

    return this->status = O2_SENSOR_STATUS_OK;
}

/**
 * @brief Reads data from the sensor
 * @param reg Register address
 * @param data Buffer to store read data
 * @param len Number of bytes to read
 * @return Status of the read operation
 */
eO2SensorStatus O2Sensor::readData(uint8_t reg, uint8_t *data, uint8_t len)
{
    uint8_t i = 0;
    _pWire->beginTransmission(I2C_ADDRESS);
    _pWire->write(reg);
    if (_pWire->endTransmission() != 0)
        return this->status = O2_SENSOR_STATUS_FAILED_TO_SEND_REQUEST;

    uint8_t bytesReceived = _pWire->requestFrom(I2C_ADDRESS, len);
    if (bytesReceived != len)
        return this->status = O2_SENSOR_STATUS_INVALID_RESPONSE;

    unsigned long startTime = millis();
    while (_pWire->available())
    {
        data[i++] = _pWire->read();
        if (millis() - startTime > 100)
            return this->status = O2_SENSOR_STATUS_TIMEOUT_EXCEEDED;
    }

    return this->status = O2_SENSOR_STATUS_OK;
}
