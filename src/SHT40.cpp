#include "SHT40.h"

/**
 * @brief Constructor to initialize the SHT40 sensor with the provided I2C interface.
 * @param wire Reference to a TwoWire object for I2C communication.
 */
SHT40::SHT40()
{
}

/**
 * @brief Get the current temperature value from the sensor.
 * @return The measured temperature in degrees Celsius.
 */
float SHT40::getTemperature()
{
    return temperature;
}

/**
 * @brief Get the current humidity value from the sensor.
 * @return The measured humidity in percentage.
 */
float SHT40::getHumidity()
{
    return humidity;
}

/**
 * @brief Check if the sensor data is valid.
 * @return True if the sensor data is valid, false otherwise.
 */
bool SHT40::isDataValid()
{
    fetchData();
    return isDataValidFlag;
}

/**
 * @brief Fetch and update the sensor data for temperature and humidity.
 */
void SHT40::fetchData()
{
    // This sequence is required to remove the 10ms delay between the write and read commands
    // Now we ask a new value at the end of the previous measurement
    if (writeCommandIsSuccessful)
    {
        // Do something
    }
    else
    {
        isDataValidFlag = false;
    }
    writeCommandIsSuccessful = writeCommand(SHT40_FETCH);
}

/**
 * @brief Write a command to the sensor.
 * @param cmd The command to send to the sensor.
 * @return true if the command was successfully written, false otherwise.
 */
bool SHT40::writeCommand(uint8_t cmd)
{
    // Do something
}

/**
 * @brief Calculate CRC8 checksum for data verification.
 * @param data Pointer to the data buffer.
 * @param len Length of the data buffer.
 * @return Calculated CRC8 checksum.
 */
uint8_t SHT40::crc8(const uint8_t *data, int len)
{
    uint8_t crc = 0xFF;
    for (int j = 0; j < len; j++)
    {
        crc ^= data[j];
        for (int i = 0; i < 8; i++)
        {
            crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
        }
    }
    return crc;
}
