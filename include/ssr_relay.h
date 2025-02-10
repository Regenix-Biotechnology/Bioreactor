#ifndef SSR_RELAY_H
#define SSR_RELAY_H

#include <Arduino.h>

/**
 * @class SSR_Relay
 * @brief Controls a solid-state relay using a PWM signal.
 *
 * This class allows setting the relay output with a PWM value between 0 and 255.
 */
class SSR_Relay
{
public:
    SSR_Relay(uint8_t pin);
    void setLevel(uint8_t level);
    void off();

private:
    uint8_t pin;
};

#endif
