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
#include "utils.h"

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
static constexpr bool OPEN = HIGH;
static constexpr bool CLOSE = LOW;
static constexpr bool ON = HIGH;
static constexpr bool OFF = LOW;
static constexpr uint8_t PUMP_MAX_SPEED = 255;
static constexpr unsigned long TEMPERATURE_CONTROLLER_UPDATE_INTERVAL = 1000;
static constexpr unsigned long SERIAL_BAUDRATE = 115200;

#endif