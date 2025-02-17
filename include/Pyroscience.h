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
    void begin();
    float getTemperature() const;
    float getPH() const;
    float getDO() const;
    bool isDataValid();

private:
    void fetchData();

    float temperature;
    float pH;
    float DO;
    bool isDataValidFlag;
};

#endif