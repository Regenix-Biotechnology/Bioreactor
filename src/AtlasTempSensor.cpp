#include "AtlasTempSensor.h"

/**
 * @brief Construct a new Atlas Temp Sensor:: Atlas Temp Sensor object
 * @param pWire Pointer to the TwoWire instance for I2C communication.
 */
AtlasTempSensor::AtlasTempSensor(TwoWire *pWire)
    : AtlasBase(pWire,
                TEMP_I2C_ADDRESS) {}

/**
 * @brief Get the latest temperature value.
 * @return float Latest temperature in Celsius.
 */
float AtlasTempSensor::getTemperatureC() const
{
    return getLastValue();
}

/**
 * @brief Calibrate the temperature sensor using a single-point calibration at 100.0C.
 * @param knownTempC The known temperature in Celsius for calibration (only 100.0C is supported).
 * @return eAtlasStatus Status of the calibration operation.
 *
 * @warning This function is blocking so it should not be called in time-critical loops.
 */
eAtlasStatus AtlasTempSensor::calibrateSinglePoint(eCalibrationValues value)
{
    ClearI2CBuffer();

    const char *cmd = nullptr;

    if (value == CAL_TEMP_100C)
        cmd = "Cal,100.00";
    else
        return ATLAS_STATUS_DEVICE_ERROR;

    if (!writeI2C(cmd, strlen(cmd), true))
    {
        return ATLAS_STATUS_FAILED_TO_SEND_REQUEST;
    }

    delay(NB_MAX_CONVERSION_MS);
    return ATLAS_STATUS_OK;
}
