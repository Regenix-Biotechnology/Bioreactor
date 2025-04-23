#ifndef PINS_H
#define PINS_H

#include <Arduino.h>

/*
*****************UPDATE the pin_usage.xlsx document when you make change to this file*****************
*/

// PUMPS
constexpr int APPROV_PUMP_PIN_1 = 14;
constexpr int APPROV_PUMP_PIN_2 = 32;
constexpr int SENSOR_PUMP_PIN_1 = 0; // not working need extender
constexpr int SENSOR_PUMP_PIN_2 = 33;
constexpr int CULTURE_CHAMBER_PUMP_1_PIN_1 = 0;  // not working need extender
constexpr int CULTURE_CHAMBER_PUMP_1_PIN_2 = 0;  // not working need extender
constexpr int CULTURE_CHAMBER_PUMP_2_PIN_1 = 13; // not working need extender
constexpr int CULTURE_CHAMBER_PUMP_2_PIN_2 = 0;  // not working need extender

// I2C
constexpr int I2C_SDA_PIN = 21;
constexpr int I2C_SCL_PIN = 22;

// PYROSCIENCE
constexpr int PYROSCIENCE_RX_PIN = 0; // not working need extender
constexpr int PYROSCIENCE_TX_PIN = 0; // not working need extender

// CO2 RS485
constexpr int RS485_RX_PIN = 16;
constexpr int RS485_TX_PIN = 17;
constexpr int RS485_DE_PIN = 4;

// RELAY
constexpr int VALVE_1_PIN = 0; // not working need extender
constexpr int VALVE_2_PIN = 0; // not working need extender
constexpr int VALVE_3_PIN = 0; // not working need extender
constexpr int VALVE_4_PIN = 0; // not working need extender
constexpr int VALVE_5_PIN = 0; // not working need extender
constexpr int HEATER_FAN_PIN = 19;
constexpr int INTERIOR_FAN_PIN = 0; // not working need extender
constexpr int EXTERIOR_FAN_PIN = 0; // not working need extender
constexpr int PATCH_HEATER_PIN = 18;
constexpr int O2_VALVE_PIN = 12;
constexpr int CO2_VALVE_PIN = 27;
constexpr int AIR_VALVE_PIN = 15;   // not working need extender
constexpr int SAFETY_VALVE_PIN = 0; // not working need extender

// SSR RELAY
constexpr int HEATER_PIN = 23;

#endif // PINS_H
