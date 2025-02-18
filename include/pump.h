#ifndef PUMP_H
#define PUMP_H

#include <Arduino.h>

/**
 * @class Pump
 * @brief Controls a pump via a Cytron motor driver using PWM.
 *
 * This class allows setting the pump speed and direction using two control pins.
 *
 * @note Cytron motor driver datasheet: https://cdn.robotshop.com/media/c/cyt/rb-cyt-260/pdf/cytron-3a-4-16v-dual-channel-dc-motor-driver-datasheet.pdf
 */
class Pump
{
public:
    Pump(uint8_t pin1, uint8_t pin2);
    void begin();
    void setSpeed(int speed);
    void stop();

private:
    uint8_t pin1;
    uint8_t pin2;
};

#endif