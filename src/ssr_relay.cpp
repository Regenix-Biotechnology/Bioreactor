#include "SSR_Relay.h"

/**
 * @brief Constructor to initialize the relay control pin.
 * @param pin The control pin for the SSR relay.
 */
SSR_Relay::SSR_Relay(uint8_t pin) : pin(pin), lastCheckTime(0) {}

/**
 * @brief Initialise SSR relay pin and set default state.
 */
void SSR_Relay::begin()
{
    pinMode(this->pin, OUTPUT);
    digitalWrite(this->pin, LOW);
}

/**
 * @brief Set the relay output level.
 * @param level A PWM value between 0 and 100.
 */
void SSR_Relay::setLevel(uint8_t level)
{
    this->levelPWM = constrain(level, 0, MAX_PWM_INDEX);
}

/**
 * @brief Turn the relay off.
 */
void SSR_Relay::off()
{
    this->setLevel(0);
}

/**
 * @brief Main loop to manage relay PWM based on temperature input.
 */
void SSR_Relay::update()
{
    unsigned long currentTime = millis();

    if (currentTime - lastCheckTime >= CHECK_INTERVAL)
    {
        if (currentPWMIndex < levelPWM)
            digitalWrite(pin, HIGH);
        else
            digitalWrite(pin, LOW);

        currentPWMIndex++;
        lastCheckTime = currentTime;

        if (currentPWMIndex >= MAX_PWM_INDEX)
            currentPWMIndex = 0;
    }
}
