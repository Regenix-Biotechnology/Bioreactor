#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include "SHT40.h"
#include "Pyroscience.h"
#include "Pump.h"
#include "ssr_relay.h"
#include "relay.h"
#include "pins.h"
#include "temperature_controller.h"
#include "bioreactor_controller.h"

enum class eBioreactorState
{
    IDLE = 0,
    APPROV,
    PREPARE,
    RUN,

    TEST,
    MAX_STATES
};

// Objects declaration (extern to be used both in main.cpp and bioreactor_controller.cpp)
extern SHT40 sht40;
extern Pyroscience pyroscience;
extern Pump approvPump;
extern Pump sensorPump;
extern Pump cultureChamberPump1;
extern Pump cultureChamberPump2;
extern SSR_Relay heater;
extern Relay heaterFan;
extern Relay interiorFan;
extern Relay patchHeater;
extern TemperatureController temperatureController;

// Global variables
extern eBioreactorState bioreactorState;
extern unsigned long lastTemperatureControllerTime;
extern uint8_t testState;

// Global constants
static constexpr bool OPEN = true;
static constexpr bool CLOSE = false;
static constexpr bool ON = true;
static constexpr bool OFF = false;
static constexpr uint8_t PUMP_MAX_SPEED = 255;
static constexpr uint16_t TEMPERATURE_CONTROLLER_UPDATE_INTERVAL = 1000;

#endif