#ifndef ATLAS_TEMP_SENSOR_H
#define ATLAS_TEMP_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include "AtlasBase.h"

/**
 * @file AtlasTempSensor.h
 * @brief Atlas Scientific EZO-RTD (temperature) circuit driver (I2C, ASCII protocol)
 *
 * @link https://atlas-scientific.com/embedded-solutions/ezo-rtd-temperature-circuit/
 */
class AtlasTempSensor : public AtlasBase
{
public:
  AtlasTempSensor(TwoWire *pWire = &Wire);
  ~AtlasTempSensor() {}

  float getTemperatureC() const;
  eAtlasStatus calibrateSinglePoint(eCalibrationValues value);

private:
  static constexpr uint8_t TEMP_I2C_ADDRESS = 0x66;
  static constexpr float NEGATIVE_TEMP_FAULT_C = -50.0f;

  bool isValueFault(float v) const { return v < NEGATIVE_TEMP_FAULT_C; }
};

#endif // ATLAS_TEMP_SENSOR_H
