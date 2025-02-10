#ifndef RELAY_H
#define RELAY_H

#include <Arduino.h>

/**
 * @class Relay
 * @brief Controls a relay module.
 *
 * This class allows turning a relay on or off using a single control pin.
 */
class Relay
{
public:
    Relay(uint8_t pin);
    void on();
    void off();

private:
    uint8_t pin;
};

#endif
