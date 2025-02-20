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
    void begin();
    void on();
    void off();
    void set(bool state);
    void toggle();
    bool getState() const;

private:
    uint8_t pin;
    bool state;
};

#endif
