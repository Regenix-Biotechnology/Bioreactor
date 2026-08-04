#ifndef CO2_CONTROLLER_H
#define CO2_CONTROLLER_H

#include <Arduino.h>

#define ARDUINO_I2C_ADDR 0x10

class CO2Controller
{
public:
    CO2Controller();
    void setReferenceLevel(float ReferenceLevel);
    void update(float co2Concentration);
    bool getValveState();
    void regulation();

private:
    float co2MinRef;
    float co2MaxRef;
    float co2Ref;
    bool isCO2ValveOpen = false;

    enum eConcentrationState
    {
        CONCENTRATION_0_to_4,
        CONCENTRATION_4_to_4_45,
        CONCENTRATION_4_45_to_4_55,
        CONCENTRATION_OVER_4_55,
        CONCENTRATION_4_55_to_4_45,
        CONCENTRATION_UDER_4_45,
    };

    eConcentrationState currentState = eConcentrationState::CONCENTRATION_0_to_4;

    static constexpr float CO2_DEAD_ZONE = 0.1f; // %
    unsigned long lastCO2UpdateValueTime = millis() + 2200;
    unsigned long LAST_CYCLE = millis();
    unsigned int IMPULSION_TIME = 300;
    unsigned int CYCLE_TIME = 10000;
};

#endif