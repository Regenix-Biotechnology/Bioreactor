
#ifndef LIMIT_SWITCH_H
#define LIMIT_SWITCH_H

#include <Arduino.h>

typedef enum
{
    DOOR_CLOSED = 0,
    DOOR_OPEN = 1
} DoorState;

/**
 * @brief Limit switch interface for door state detection.
 */
class LimitSwitch
{
public:
    /**
     * @brief Constructor
     * @param pin The GPIO pin connected to the limit switch.
     */
    LimitSwitch(uint8_t pin) : _pin(pin) {}

    /**
     * @brief Initialize the limit switch pin.
     */
    void begin()
    {
        pinMode(_pin, INPUT_PULLUP);
    }

    /**
     * @brief Get the current state of the door.
     * @param pin The GPIO pin connected to the limit switch.
     * @return DoorState The current state of the door.
     */
    DoorState getDoorState()
    {
        return (digitalRead(_pin) == LOW) ? DOOR_CLOSED : DOOR_OPEN;
    }

private:
    uint8_t _pin;
};

#endif // LIMIT_SWITCH_H
