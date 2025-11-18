
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
     * @brief Initialize I2C as master.
     */
    void begin(TwoWire *i2cBus)
    {
        _wire = i2cBus;
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

private:
    TwoWire *_wire;
    static constexpr uint8_t I2C_ADDRESS = 0x10;
};
#endif // LED_I2C_H
