
#ifndef LED_I2C_H
#define LED_I2C_H

#include <Arduino.h>
#include <Wire.h>

typedef enum
{
    LED_STATE_IDLE = 0,
    LED_STATE_DOOR_OPEN,
    LED_STATE_ERROR,

    LED_STATE_MAX
} eLedState;

/**
 * @brief I2C master communication for sending state to Arduino over I2C
 */
class LedI2C
{
public:
    /**
     * @brief Constructor to initialize the I2C communication with the Arduino used for LED control.
     * @param i2cBus Pointer to the TwoWire instance to use.
     */
    LedI2C(TwoWire *i2cBus)
        : _wire(i2cBus)
    {
    }

    /**
     * @brief Send a state byte to the Arduino over I2C.
     * @param state The state byte to send (0-9).
     */
    void sendState(eLedState state)
    {
        _wire->beginTransmission(I2C_ADDRESS);
        _wire->write(state);
        _wire->endTransmission();
    }

    float getCO2Value()
    {
        float co2Received = 0.0f;
        uint8_t buffer[sizeof(float)];

        // 1. Demande des octets à l'esclave
        size_t bytesRead = _wire->requestFrom((uint8_t)I2C_ADDRESS, (uint8_t)sizeof(float));

        if (bytesRead == sizeof(float))
        {
            for (size_t i = 0; i < sizeof(float); i++)
            {
                buffer[i] = _wire->read();
            }

            // --- DEBUG: Inscription des octets bruts reçus ---
            Serial.print("[I2C RAW]: ");
            for (size_t i = 0; i < sizeof(float); i++)
            {
                Serial.print("0x");
                if (buffer[i] < 0x10)
                    Serial.print("0");
                Serial.print(buffer[i], HEX);
                Serial.print(" ");
            }
            Serial.println();

            // Transfert sécurisé dans le float
            memcpy(&co2Received, buffer, sizeof(float));

            // En cas de réécriture si l'endianness est inversé (testez ceci si les octets sont valides)
            /*
            uint8_t reversed[4] = { buffer[3], buffer[2], buffer[1], buffer[0] };
            memcpy(&co2Received, reversed, sizeof(float));
            */

            return co2Received;
        }

        // Vidage du buffer si problème de transmission
        while (_wire->available() > 0)
        {
            _wire->read();
        }

        Serial.println("[I2C Error]: Impossible de lire 4 octets.");
        return -1.0f; // Valeur sentinelle d'erreur
    }

private:
    TwoWire *_wire;
    static constexpr uint8_t I2C_ADDRESS = 0x10;
};
#endif // LED_I2C_H
