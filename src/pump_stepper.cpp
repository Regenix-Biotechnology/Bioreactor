#include "pump_stepper.h"

PumpStepper::PumpStepper(uint8_t stepPin, uint8_t dirPin)
/*: stepper(AccelStepper::DRIVER, stepPin, dirPin)*/
{
}

void PumpStepper::begin()
{
    // stepper.setMaxSpeed(MAX_SPEED);
    // stepper.setAcceleration(ACCELERATION);
}

void PumpStepper::setSpeed(int speed)
{
    // stepper.setSpeed(speed);
}

void PumpStepper::update()
{
    // stepper.runSpeed();
}

void PumpStepper::stop()
{
    // stepper.setSpeed(0);
}