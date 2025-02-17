#include "SHT40.h"

/**
 * @brief Constructor to initialize the SHT40 sensor with the provided I2C interface.
 * @param wire Reference to a TwoWire object for I2C communication.
 */
SHT40::SHT40() {}

/**
 * @brief initialise sht40
 *
 */
void SHT40::begin() {}

/**
 * @brief Get the current temperature value from the sensor.
 * @return The measured temperature in degrees Celsius.
 */
float SHT40::getTemperature() const
{
    return temperature;
}

/**
 * @brief Get the current humidity value from the sensor.
 * @return The measured humidity in percentage.
 */
float SHT40::getHumidity() const
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
    // Fake data for the moment. TODO: Implement actual data fetching from the sensor.
    temperature = 25.0;
    humidity = 50.0;
    isDataValidFlag = true;
}