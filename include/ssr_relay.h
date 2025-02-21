#ifndef SSR_RELAY_H
#define SSR_RELAY_H

#include <Arduino.h>

/**
 * @class SSR_Relay
 * @brief Controls a solid-state relay using a PWM signal.
 *
 * This class allows setting the relay output with a PWM value between 0 and 100.
 */
class SSR_Relay
{
public:
    SSR_Relay(uint8_t pin);
    void begin();
    void setLevel(uint8_t level);
    void update();
    void off();

private:
    static constexpr unsigned long CHECK_INTERVAL = 10;
    static constexpr uint8_t MAX_PWM_INDEX = 100;
    unsigned long lastCheckTime;
    uint8_t levelPWM;
    uint8_t pin;
    uint8_t currentPWMIndex;
};

#endif
