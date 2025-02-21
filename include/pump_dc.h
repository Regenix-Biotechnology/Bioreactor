#ifndef PUMP_DC_H
#define PUMP_DC_H

#include <pump.h>

/**
 * @class Pump
 * @brief Controls a pump via a Cytron motor driver using PWM.
 *
 * This class allows setting the pump speed and direction using two control pins.
 *
 * @note Cytron motor driver datasheet: https://cdn.robotshop.com/media/c/cyt/rb-cyt-260/pdf/cytron-3a-4-16v-dual-channel-dc-motor-driver-datasheet.pdf
 */
class PumpDC : public Pump
{
public:
    PumpDC(uint8_t pin1, uint8_t pin2);
    void begin() override;
    void setSpeed(int speed) override;
    void stop() override;

private:
    uint8_t pin1, pin2;
};

#endif