#ifndef SHT40_H
#define SHT40_H

#include <Wire.h>

/**
 * @class SHT40
 * @brief A class to interface with the SHT40 temperature and humidity sensor.
 *
 * This class allows reading temperature and humidity data from the SHT40 sensor over I2C.
 *
 * @note SHT40 datasheet: https://sensirion.com/media/documents/33FD6951/662A593A/HT_DS_Datasheet_SHT4x.pdf
 */
class SHT40
{
public:
    SHT40();
    void begin();
    float getTemperature() const;
    float getHumidity() const;
    bool isDataValid();

private:
    void fetchData();

    float temperature;
    float humidity;
    bool isDataValidFlag;
};

#endif // SHT40_H