#include "SSR_Relay.h"

/**
 * @brief Constructor to initialize the relay control pin.
 * @param pin The control pin for the SSR relay.
 */
SSR_Relay::SSR_Relay(uint8_t pin) : pin(pin) {}

/**
 * @brief Initialise SSR relay pin and set default state.
 */
void SSR_Relay::begin()
{
    pinMode(this->pin, OUTPUT);   // Initialiser la broche du relais en mode sortie
    digitalWrite(this->pin, LOW); // Initialiser le relais avec un PWM de 0 (éteint)
    lastCheckTime = 0;
}

/**
 * @brief Set the relay output level.
 * @param level A PWM value between 0 and 100.
 */
void SSR_Relay::setLevel(uint8_t level)
{
    this->levelPWM = constrain(level, 0, 100);
}

/**
 * @brief Turn the relay off.
 */
void SSR_Relay::off()
{
    digitalWrite(pin, LOW); // Désactive le relais en envoyant 0 au PWM
}

/**
 * @brief Main loop to manage relay PWM based on temperature input.
 */
void SSR_Relay::update()
{

    // Vérification de la période PWM toutes les 10ms
    unsigned long currentTime = millis();

    if (currentTime - lastCheckTime >= CHECK_INTERVAL)
    {
        if (currentPWM < levelPWM)
        {
            digitalWrite(pin, HIGH);
            // Serial.println("HIGH");
        }
        else
        {
            digitalWrite(pin, LOW);
            // Serial.println("LOW");
        }
        currentPWM++;
        lastCheckTime = currentTime;

        // Vérifier la période PWM et ajuster le signal en conséquence
        if (currentPWM > 100)
        {
            currentPWM = 0;
        }
    }
}
