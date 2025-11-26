#ifndef TEMPERATURE_CONTROLLER_H
#define TEMPERATURE_CONTROLLER_H

#include <Arduino.h>

/**
 * @class TemperatureController
 * @brief Implements the control loop for temperature regulation using a PID algorithm.
 *
 * The control loop computes:
 * - A target air temperature based on the water temperature.
 * - A PID controller computes the fan power (with limits).
 */
class TemperatureController
{
public:
    TemperatureController();
    void update(float waterTemp, float airTemp);
    float getHeaterPower() const;
    void setReferenceTemperature(float tempRef);

private:
    // Reference temperature.
    float tempRef;

    // Internal state for the PID controller.
    float integralError;
    float prevError;
    unsigned long prevTime;
    float integralErrorAir;

    // Control outputs.
    uint8_t pwmHeater;

    // Constants for the control loop.
    static constexpr float KP_FAN = 20.0f;
    static constexpr float KI_FAN = 0.0001f;
    static constexpr float KD_FAN = 0.0f;
    static constexpr float KP_AIR = 4.0f;
    static constexpr float KI_AIR = 0.0005f;
    static constexpr float MILLIS_TO_SECONDS = 1000.0f;
};

#endif // TEMPERATURE_CONTROLLER_H
