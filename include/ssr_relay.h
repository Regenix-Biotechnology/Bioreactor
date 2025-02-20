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
    void begin();
    void setLevel(uint8_t level);
    void update();
    void off();

private:
    unsigned long lastCheckTime; // Dernière vérification
    uint8_t levelPWM;
    uint8_t pin;
    uint8_t currentPWM;
    static constexpr unsigned long checkInterval = 10; // Intervalle de vérification en millisecondes (10ms)
};

#endif
