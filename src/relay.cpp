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
    this->state = false;
}

/**
 * @brief Turn the relay on.
 */
void Relay::on()
{
    digitalWrite(pin, HIGH);
    this->state = true;
}

/**
 * @brief Turn the relay off.
 */
void Relay::off()
{
    digitalWrite(pin, LOW);
    this->state = false;
}

/**
 * @brief Set the state of the relay.
 * @param state true to turn the relay on, false to turn it off.
 */
void Relay::set(bool state)
{
    state ? this->on() : this->off();
}

/**
 * @brief Toggle the state of the relay.
 */
void Relay::toggle()
{
    this->set(!this->state);
}

/**
 * @brief Get the current state of the relay.
 * @return true if the relay is on, false otherwise.
 */
bool Relay::getState() const
{
    return this->state;
}
