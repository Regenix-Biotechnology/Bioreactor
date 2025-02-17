#include "Pump.h"

/**
 * @brief Constructor to initialize the pump control pins.
 * @param pin1 The first control pin.
 * @param pin2 The second control pin.
 */
Pump::Pump(uint8_t pin1, uint8_t pin2) : pin1(pin1), pin2(pin2) {}

/**
 * @brief initialise pump pin
 *
 */
void Pump::begin()
{
    pinMode(this->pin1, OUTPUT);
    pinMode(this->pin2, OUTPUT);
}

/**
 * @brief Set the pump speed and direction.
 * @param speed A value between -255 and 255 (- for reverse, + for forward).
 */
void Pump::setSpeed(int speed)
{
    // TODO: Change the range from -255 to 255 to a value in ml/min.
    speed = constrain(speed, -255, 255);
    if (speed > 0)
    {
        analogWrite(pin1, speed);
        analogWrite(pin2, 0);
    }
    else if (speed < 0)
    {
        analogWrite(pin1, 0);
        analogWrite(pin2, -speed);
    }
    else
    {
        stop();
    }
}

/**
 * @brief Stop the pump.
 */
void Pump::stop()
{
    analogWrite(pin1, 0);
    analogWrite(pin2, 0);
}
