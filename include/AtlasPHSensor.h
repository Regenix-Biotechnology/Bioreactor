#ifndef ATLAS_PH_SENSOR_H
#define ATLAS_PH_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include "AtlasBase.h"

/**
 * @file AtlasPHSensor.h
 * @brief Atlas Scientific EZO-pH circuit driver (I2C, ASCII protocol)
 *
 * @link https://atlas-scientific.com/embedded-solutions/ezo-ph-circuit/
 */
class AtlasPHSensor : public AtlasBase
{
public:
  AtlasPHSensor(TwoWire *pWire = &Wire);
  ~AtlasPHSensor() {}

  float getPH() const;
  eAtlasStatus calibrateSinglePoint(eCalibrationValues value);

private:
  static constexpr uint8_t PH_I2C_ADDRESS = 0x63;
};

#endif // ATLAS_PH_SENSOR_H
