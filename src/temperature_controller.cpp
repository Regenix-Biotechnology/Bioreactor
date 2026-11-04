#include "temperature_controller.h"
#include <math.h>

/**
 * @brief Constructor to initialize the control loop parameters.
 */
TemperatureController::TemperatureController()
    : integralError(0.0f),
      prevError(0.0f),
      prevTime(0),
      integralErrorAir(0.0f),
      pwmHeater(0),
      tempRef(37.0f)
{
}

/**
 * @brief Updates the control loop with the latest temperature measurements.
 * @param waterTemp Current water temperature (°C).
 * @param airTemp   Current air temperature (°C).
 *
 * @note This function needs to be called at 1 Hz.
 */
void TemperatureController::update(float waterTemp, float airTemp)
{
    if (!isfinite(waterTemp) || !isfinite(airTemp) || !isfinite(tempRef))
    {
        reset();
        return;
    }

    // --- Compute Target Air Temperature ---
    unsigned long currentTime = millis();
    float dt = (float)(currentTime - prevTime) / MILLIS_TO_SECONDS;
    if (dt <= 0.0f)
    {
        reset();
        return;
    }

    float error = (tempRef - waterTemp);
    if (error <= 0.0f)
    {
        reset();
        return;
    }

    this->integralErrorAir += error * dt;

    float targetAirTemp = KP_AIR * error + KI_AIR * integralErrorAir + tempRef - TEMPERATURE_DIFFERENCE_OFFSET;

    // --- PID Control for the Heater ---
    error = targetAirTemp - airTemp;
    float derivative = (error - prevError) / dt;
    this->integralError += error * dt;
    this->prevError = error;
    this->prevTime = currentTime;

    float heaterControl = KP_HEATER * error + KI_HEATER * integralError + KD_HEATER * derivative;
    this->pwmHeater = constrain(heaterControl, 0, 100);

    // --- Debug Output ---
    // Serial.print("Target Air Temp: ");
    // Serial.print(targetAirTemp);
    // Serial.print(" Air Temp: ");
    // Serial.print(airTemp);
    // Serial.print(" Water Temp: ");
    // Serial.print(waterTemp);
    // Serial.print(" Heater Control: ");
    // Serial.print(heaterControl);
    // Serial.print(" Heater Power: ");
    // Serial.print(this->pwmHeater);
}

/**
 * @brief Clear accumulated controller state and stop the heater.
 */
void TemperatureController::reset()
{
    this->integralError = 0.0f;
    this->integralErrorAir = 0.0f;
    this->prevError = 0.0f;
    this->prevTime = millis();
    this->pwmHeater = 0;
}

/**
 * @brief Returns the computed heater power between 0 and 100 percent.
 */
float TemperatureController::getHeaterPower() const
{
    return this->pwmHeater;
}

/**
 * @brief Set the reference temperature for the control loop.
 * @param tempRef The reference temperature (°C).
 */
void TemperatureController::setReferenceTemperature(float tempRef)
{
    if (this->tempRef != tempRef)
        reset();
    this->tempRef = tempRef;
}

/**
 * @brief Get the reference temperature for the control loop.
 * @return tempRef The reference temperature (°C).
 */
float TemperatureController::getReferenceTemperature()
{
    return this->tempRef;
}