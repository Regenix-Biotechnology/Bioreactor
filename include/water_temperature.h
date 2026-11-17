#ifndef WATER_TEMPERATURE_H
#define WATER_TEMPERATURE_H

#include <Arduino.h>

/**
 * @brief Selection of the water temperature used by the heater regulation.
 *
 * Two sources measure the medium temperature:
 *  - ATLAS : Atlas EZO-RTD probe (tempSensor), main source when it is healthy;
 *  - DO    : temperature measured by the VisiFerm DO probe (dissolvedOxygenSensor).
 *
 * Rules, evaluated once per second by updateWaterTemperature():
 *  1. ATLAS is used if its reading is valid, plausible (5–60 °C), has no jump > 2 °C in 1 s,
 *     and agrees with the VisiFerm within 2 °C (when the VisiFerm is available).
 *  2. Otherwise the VisiFerm temperature is used (reading < 5 s old and plausible).
 *  3. If neither is usable: no valid water temperature -> the heater is switched off.
 *  After a fallback to DO, ATLAS is used again only after 10 s of consecutive good readings.
 *
 * Each change of source is reported: EVENT:TEMP:SOURCE=<ATLAS|DO|NONE>;REASON=<...>
 * (and ERR:TEMP:NO_VALID_SENSOR when no source is left).
 */

/** @brief Re-evaluate the water temperature source. Call at 1 Hz (done in updateTemperatureController()). */
void updateWaterTemperature();

/** @brief true if a usable water temperature is available. */
bool isWaterTemperatureValid();

/** @brief Water temperature (°C) from the selected source, NAN if none. */
float getWaterTemperature();

/** @brief "ATLAS", "DO" or "NONE". */
const char *getWaterTemperatureSourceName();

#endif // WATER_TEMPERATURE_H