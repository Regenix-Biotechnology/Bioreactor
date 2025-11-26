#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <Preferences.h>
#include "SHT40.h"
#include "stepper_motor.h"
#include "ssr_relay.h"
#include "ioExpander.h"
#include "pins.h"
#include "temperature_controller.h"
#include "bioreactor_controller.h"
#include "watchdog.h"
#include "utils.h"
#include "pressure_chamber_controller.h"
#include "O2Sensor.h"
#include "gmp251.h"
#include "visiferm_RS485.h"
#include "AtlasPHSensor.h"
#include "AtlasTempSensor.h"
#include "limitSwitch.h"
#include "ledI2C.h"

enum class eBioreactorState
{
    IDLE = 0,
    APPROV,
    PREPARE,
    RUN,
    CELL_RETURN,
    CLEANING_APPROV,
    CLEANING_CIRCULATION,
    CLEANING_RETURN,
    RINSING_APPROV,
    RINSING_CIRCULATION,
    RINSING_RETURN,
    REDUCE_OVERFLOW,
    TEST,
    OPEN_VALVES,
    SAMPLING,

    MAX_STATE
};

// Objects declaration (extern to be used both in main.cpp and bioreactor_controller.cpp)
extern SHT40 sht40;
extern DriveTmc5041 driveStepper1;
extern DriveTmc5041 driveStepper3;
extern StepperMotor approvPump;
extern StepperMotor circulationPump;
extern StepperMotor cultureChamberPump1;
extern StepperMotor cultureChamberPump2;
extern SSR_Relay heater;
extern IOExpander ioExpander;
extern TemperatureController temperatureController;
extern VisiFermRS485 dissolvedOxygenSensor;
extern AtlasPHSensor pHSensor;
extern AtlasTempSensor tempSensor;
extern GMP251 co2Sensor;
extern LimitSwitch limitSwitch;
extern LedI2C ledI2C;
extern Preferences bioreactorParameter;
extern PressureChamberController pressureChamber;

// Global variables
extern eBioreactorState bioreactorState;
extern unsigned long lastTemperatureControllerTime;
extern unsigned long lastPrintTime;
extern uint8_t testState;
extern unsigned long stateTimer;

// Global constants
static constexpr bool OPEN = HIGH;
static constexpr bool CLOSE = LOW;
static constexpr bool ON = HIGH;
static constexpr bool OFF = LOW;
static constexpr uint8_t PUMP_MAX_SPEED = 255;
static constexpr unsigned long TEMPERATURE_CONTROLLER_UPDATE_INTERVAL = 1000;
static constexpr unsigned long MINUTE = 60000;
static constexpr unsigned long PRINT_UPDATE_INTERVAL = 1000;
static constexpr unsigned long PRESSURE_CHAMBER_CONTROLLER_UPDATE_INTERVAL = 60000; // Based on the GMP251 response time
static constexpr unsigned long MOTOR_SET_SPEED_MSG_INTERVAL = 1250;
static constexpr unsigned long LED_UPDATE_INTERVAL = 1000;
static constexpr unsigned long SERIAL_BAUDRATE = 115200;

#endif