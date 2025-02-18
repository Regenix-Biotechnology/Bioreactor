#include "temperature_controller.h"

/**
 * @brief Constructor to initialize the control loop parameters.
 */
TemperatureController::TemperatureController()
    : integralError(0.0f),
      prevError(0.0f),
      pwmHeater(0),
      patchState(false),
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
    // --- Activation of Patch Heating ---
    this->patchState = waterTemp < (tempRef - TEMPERATURE_REFERENCE_OFFSET);

    // --- Compute Target Air Temperature ---
    float targetAirTemp = KP_AIR * (waterTemp - tempRef) + tempRef;

    // --- PID Control for the Heater ---
    unsigned long currentTime = millis();
    float dt = (float)(currentTime - prevTime) / MILLIS_TO_SECONDS;
    float error = targetAirTemp - airTemp;
    float derivative = (error - prevError) / dt;
    this->integralError += error * dt;
    this->prevError = error;
    this->prevTime = currentTime;

    float heaterControl = KP_FAN * error + KI_FAN * integralError + KD_FAN * derivative;
    this->pwmHeater = constrain(heaterControl, 0, 255);
}

/**
 * @brief Returns the computed fan power pwm between 0 and 255.
 */
float TemperatureController::getHeaterPower() const
{
    return this->pwmHeater;
}

/**
 * @brief Returns if the patch needs to be activated.
 */
bool TemperatureController::isPatchHeatingNeeded() const
{
    return this->patchState;
}

/**
 * @brief Set the reference temperature for the control loop.
 * @param tempRef The reference temperature (°C).
 */
void TemperatureController::setReferenceTemperature(float tempRef)
{
    this->tempRef = tempRef;
}