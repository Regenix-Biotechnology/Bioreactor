#ifndef PINS_H
#define PINS_H

#include <Arduino.h>

// PUMPS
constexpr int APPROV_PUMP_PIN_1 = 3;
constexpr int APPROV_PUMP_PIN_2 = 5;
constexpr int SENSOR_PUMP_PIN_1 = 6;
constexpr int SENSOR_PUMP_PIN_2 = 9;
constexpr int CULTURE_CHAMBER_PUMP_1_PIN_1 = 10;
constexpr int CULTURE_CHAMBER_PUMP_1_PIN_2 = 11;
constexpr int CULTURE_CHAMBER_PUMP_2_PIN_1 = 12;
constexpr int CULTURE_CHAMBER_PUMP_2_PIN_2 = 13;

// SHT40
constexpr int SHT40_SDA_PIN = 20;
constexpr int SHT40_SCL_PIN = 21;

// PYROSCIENCE
constexpr int PYROSCIENCE_RX_PIN = 16;
constexpr int PYROSCIENCE_TX_PIN = 17;

// RELAY
constexpr int VALVE_1_PIN = 2;
constexpr int VALVE_2_PIN = 4;
constexpr int VALVE_3_PIN = 7;
constexpr int VALVE_4_PIN = 8;
constexpr int VALVE_5_PIN = 14;
constexpr int HEATER_FAN_PIN = 15;
constexpr int INTERIOR_FAN_PIN = 18;
constexpr int EXTERIOR_FAN_PIN = 19;
constexpr int PATCH_HEATER_PIN = 22;

// SSR RELAY
constexpr int HEATER_PIN = 23;

#endif // PINS_H
