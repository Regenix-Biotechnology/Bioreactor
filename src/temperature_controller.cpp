#include "temperature_controller.h"

/**
 * @brief Constructor to initialize the control loop parameters.
 */
TemperatureController::TemperatureController()
    : integral_error(0.0f),
      prev_error(0.0f),
      pwmHeater(0.0f),
      statePatch(0.0f),
      targetAirTemp(0.0f),
      T_ref(37.0f)
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
    statePatch = waterTemp < (T_ref - TEMPERATURE_REFERENCE_OFFSET);

    // --- Compute Target Air Temperature ---
    targetAirTemp = KP_AIR * (waterTemp - T_ref) + T_ref;

    // --- PID Control for the Heater ---
    unsigned long current_time = millis();
    float dt = (current_time - prev_time) / MILLIS_TO_SECONDS;
    float error = targetAirTemp - airTemp;
    float derivative = (error - prev_error) / dt;
    integral_error += error * dt;
    prev_error = error;
    prev_time = current_time;

    float heaterControl = KP_FAN * error + KI_FAN * integral_error + KD_FAN * derivative;
    pwmHeater = constrain(heaterControl, 0.0f, 255.0f);
}

/**
 * @brief Returns the computed fan power pwm between 0 and 255.
 */
float TemperatureController::getHeaterPower() const
{
    return pwmHeater;
}

/**
 * @brief Returns if the patch needs to be activated.
 */
bool TemperatureController::getPatchState() const
{
    return statePatch;
}

/**
 * @brief Set the reference temperature for the control loop.
 * @param T_ref The reference temperature (°C).
 */
void TemperatureController::setReferenceTemperature(float T_ref)
{
    this->T_ref = T_ref;
}