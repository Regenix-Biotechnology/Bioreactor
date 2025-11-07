#ifndef PUMP_H
#define PUMP_H

#include <Arduino.h>

/**
 * @class Pump
 * @brief This is the parent class for all pump types
 */
class Pump
{
public:
    virtual void begin() = 0;
    virtual void setSpeed(int speed) = 0;
    virtual void stop() = 0;
    virtual void update() {};
};

#endif // PUMP_H