#include "SSR_Relay.h"

/**
 * @brief Constructor to initialize the relay control pin.
 * @param pin The control pin for the SSR relay.
 */
SSR_Relay::SSR_Relay(uint8_t pin) : pin(pin)
{
    pinMode(pin, OUTPUT);
    analogWrite(pin, 0);
}

/**
 * @brief Set the relay output level.
 * @param level A PWM value between 0 and 255.
 */
void SSR_Relay::setLevel(uint8_t level)
{
    analogWrite(pin, level);
}

/**
 * @brief Turn the relay off.
 */
void SSR_Relay::off()
{
    analogWrite(pin, 0);
}
