#include "Pyroscience.h"

/**
 * @brief Constructor to initialize the Pyroscience sensor
 */
Pyroscience::Pyroscience() {}

/**
 * @brief initialise pyroscience
 *
 */
void Pyroscience::begin()
{
    // pSerial.begin()...
}

/**
 * @brief Get the current temperature value from the sensor.
 * @return The measured temperature in degrees Celsius.
 */
float Pyroscience::getTemperature() const
{
    return temperature;
}

/**
 * @brief Get the current pH value from the sensor.
 * @return The measured pH value.
 */
float Pyroscience::getPH() const
{
    return pH;
}

/**
 * @brief Get the current dissolved oxygen value from the sensor.
 * @return The measured dissolved oxygen value in mg/L.
 */
float Pyroscience::getDO() const
{
    return DO;
}

/**
 * @brief Check if the sensor data is valid.
 * @return True if the sensor data is valid, false otherwise.
 */
bool Pyroscience::isDataValid()
{
    fetchData();
    return isDataValidFlag;
}

/**
 * @brief Fetch and update the sensor data for temperature, pH, and dissolved oxygen.
 */
void Pyroscience::fetchData()
{
    // Fake data for the moment. TODO: Implement actual data fetching from the sensor.
    temperature = 25.0;
    pH = 7.0;
    DO = 5.0;
    isDataValidFlag = true;
}
