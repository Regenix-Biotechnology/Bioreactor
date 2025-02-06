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
    float getTemperature();
    float getHumidity();
    bool isDataValid();

private:
    void fetchData();
    bool writeCommand(uint8_t cmd);
    uint8_t crc8(const uint8_t *data, int len);

    float temperature;
    float humidity;
    bool isDataValidFlag;
    bool writeCommandIsSuccessful;

    /*
    Currently set to low repeatability (table 8) for minimum power usage (table 4)
    High repeatability    -  0xFD  -  2.2 uA
    Medium repeatability  -  0xF6  -  1.2 uA
    Low repeatability     -  0xE0  -  0.4 uA
    */
    static constexpr uint8_t SHT40_FETCH = 0xE0;
    static constexpr uint8_t SHT40_ADDRESS = 0x44;
};

#endif // SHT40_H