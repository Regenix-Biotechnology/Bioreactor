#include "Pyroscience.h"

/**
 * @brief Constructor to initialize the Pyroscience sensor
 */
Pyroscience::Pyroscience() {}

/**
 * @brief Get the current temperature value from the sensor.
 * @return The measured temperature in degrees Celsius.
 */
float Pyroscience::getTemperature()
{
    return 0.0f;
}

/**
 * @brief Get the current pH value from the sensor.
 * @return The measured pH value.
 */
float Pyroscience::getPH()
{
    return 0.0f;
}

/**
 * @brief Get the current dissolved oxygen value from the sensor.
 * @return The measured dissolved oxygen value in mg/L.
 */
float Pyroscience::getDO()
{
    return 0.0f;
}

/**
 * @brief Check if the sensor data is valid.
 * @return True if the sensor data is valid, false otherwise.
 */
bool Pyroscience::isDataValid()
{
    return false;
}

/**
 * @brief Fetch and update the sensor data for temperature, pH, and dissolved oxygen.
 */
void Pyroscience::fetchData() {}

/**
 * @brief Write a command to the sensor.
 * @param cmd The command to send to the sensor.
 * @return true if the command was successfully written, false otherwise.
 */
bool Pyroscience::writeCommand(uint8_t cmd)
{
    return false;
}
