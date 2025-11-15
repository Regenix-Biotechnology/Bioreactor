#include "AtlasPHSensor.h"

AtlasPHSensor::AtlasPHSensor(TwoWire *pWire)
    : AtlasBase(pWire, PH_I2C_ADDRESS) {}

/**
 * @brief Get the latest pH value read from the sensor.
 * @return float Latest pH value.
 */
float AtlasPHSensor::getPH() const
{
    return getLastValue();
}

/**
 * @brief Calibrate the pH sensor using a single-point calibration.
 * @param knownValue The known pH value for calibration (typically 4.00, 7.00, or 10.00).
 * @return eAtlasStatus Status of the calibration operation.
 *
 * @warning The 7.00 cal clears all other calibration, so you need to calibrate the 7.00 pH point before calibrating other points.
 * @warning This function is blocking so it should not be called in time-critical loops.
 */
eAtlasStatus AtlasPHSensor::calibrateSinglePoint(eCalibrationValues value)
{
    ClearI2CBuffer();

    const char *cmd = nullptr;

    if (value == CAL_PH_7)
        cmd = "Cal,mid,7.00";
    else if (value == CAL_PH_4)
        cmd = "Cal,low,4.00";
    else if (value == CAL_PH_10)
        cmd = "Cal,high,10.00";
    else
        return ATLAS_STATUS_DEVICE_ERROR;

    if (!writeI2C(cmd, strlen(cmd), true))
        return ATLAS_STATUS_FAILED_TO_SEND_REQUEST;

    delay(NB_MAX_CONVERSION_MS);
    return ATLAS_STATUS_OK;
}
