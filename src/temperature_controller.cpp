#include "temperature_controller.h"

/**
 * @brief Constructor to initialize the control loop parameters.
 */
TemperatureController::TemperatureController()
    : integralError(0.0f),
      prevError(0.0f),
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
    // --- Compute Target Air Temperature ---
    unsigned long currentTime = millis();
    float dt = (float)(currentTime - prevTime) / MILLIS_TO_SECONDS;
    float error = (tempRef - waterTemp);
    this->integralErrorAir += error * dt;

    float adjustedKP_AIR = KP_AIR;
    if (error < 0) // More aggressive for cooling
        adjustedKP_AIR = KP_AIR * 3.0f;

    float targetAirTemp = adjustedKP_AIR * error + KI_AIR * integralErrorAir + tempRef;

    // --- PID Control for the Heater ---
    error = targetAirTemp - airTemp;
    float derivative = (error - prevError) / dt;
    this->integralError += error * dt;
    this->prevError = error;
    this->prevTime = currentTime;

    float heaterControl = KP_FAN * error + KI_FAN * integralError + KD_FAN * derivative;
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
 * @brief Returns the computed fan power pwm between 0 and 255.
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
    this->tempRef = tempRef;
}