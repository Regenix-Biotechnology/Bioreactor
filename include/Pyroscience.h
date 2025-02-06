#ifndef PYROSCIENCE_H
#define PYROSCIENCE_H

#include <Arduino.h>

/**
 * @class Pyroscience
 * @brief A class to interface with the Pyroscience FireSting sensor.
 *
 * This class allows reading temperature, pH, and dissolved oxygen data from the Pyroscience FireSting sensor over UART
 *
 * @note Pyroscience FireSting datasheet: https://www.pyroscience.com/en/downloads/laboratory-devices
 */
class Pyroscience
{
public:
    Pyroscience();
    float getTemperature();
    float getPH();
    float getDO();
    bool isDataValid();

private:
    void fetchData();
    bool writeCommand(uint8_t cmd);
};

#endif