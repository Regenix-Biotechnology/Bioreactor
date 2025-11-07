#ifndef PUMP_STEPPER_H
#define PUMP_STEPPER_H

#include "Pump.h"
#include <AccelStepper.h>

/**
 * @class Pump
 * @brief Controls a pump stepper motor using AccelStepper library.
 *
 * This class allows setting the pump speed and direction using two control pins.
 *
 * @note Motor driver datasheet: https://www.amazon.ca/-/fr/TB6600-Pilote-Contr%C3%B4leur-segments-Hybride/dp/B07SBZ9SM5/ref=sr_1_4?__mk_fr_CA=%C3%85M%C3%85%C5%BD%C3%95%C3%91&crid=12ATP3DQXORNA&dib=eyJ2IjoiMSJ9.A7q5aElqRrfEh7-p70RTKhk-Ihe-FtkrkCd0zEaFjIkHjYI4i3VJVI7IPvX5P0q3jb04KXOPWkYORVG3FQ_1bfNyo1k2e7FTbtT8whxYAi6qd2RQRUKbdA7PITiWnCwitISjWvj4oi54-7xGXvi6RkPcbIRA7eEcu63iv7NQmf2GOMMJ3icyaObDK7x1RwSYlN5o8-g8PkYb_CtMfh4DcY0fZDeMdydNR2fRstw5hFHQSF_CzrPimqY4zKqfQBC0EzSGgaGBXv1GubT6hn0RaUcWwjkpd7jcH3SPavd0wl_3BV-ykBkaTLVi5jxoX_zJV3GpNHVysAv0s-4o-ResYIBqlUGzwpMkJrnQ3SS4thI.YhZi4h6Pmjrf2hd7TyrCTG70rzJg1VcXbcDxZEFMv0w&dib_tag=se&keywords=stepper+motor+controller&qid=1738607950&s=hi&sprefix=stepper+motor+controller%2Ctools%2C82&sr=1-4
 */
class PumpStepper : public Pump
{
public:
    PumpStepper(uint8_t stepPin, uint8_t dirPin);
    void begin() override;
    void setSpeed(int speed) override;
    void stop() override;
    void update() override;

    static constexpr int MAX_SPEED = 1000; // Maximum speed (steps per second)

private:
    AccelStepper stepper;

    static constexpr int ACCELERATION = 500;
};

#endif // PUMP_STEPPER_H