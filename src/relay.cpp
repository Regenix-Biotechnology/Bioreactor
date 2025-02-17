#include "Relay.h"

/**
 * @brief Constructor to initialize the relay control pin.
 * @param pin The control pin for the relay.
 */
Relay::Relay(uint8_t pin) : pin(pin) {}

/**
 * @brief initialise relay pin and set default state
 *
 */
void Relay::begin()
{
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

/**
 * @brief Turn the relay on.
 */
void Relay::on()
{
    digitalWrite(pin, HIGH);
}

/**
 * @brief Turn the relay off.
 */
void Relay::off()
{
    digitalWrite(pin, LOW);
}