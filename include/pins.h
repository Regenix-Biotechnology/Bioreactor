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
constexpr int RS485_DE_PIN = 15;

// eFuses (via IO Expander)

constexpr uint8_t EFUSE_PUMP_APPROV_PIN = 0;  // ex: IOExpander P0_0
constexpr uint8_t EFUSE_PUMP_CIRCUIT_PIN = 1; // ex: IOExpander P0_1
constexpr uint8_t EFUSE_PUMP_CULTURE_CHAMBER_1_PIN = 2;
constexpr uint8_t EFUSE_PUMP_CULTURE_CHAMBER_2_PINN = 3;

constexpr uint8_t EFUSE_VALVE_APPROV_PIN = 4;
constexpr uint8_t EFUSE_VALVE_CIRCULATION_PIN = 5;
constexpr uint8_t EFUSE_VALVE_RETOUR_PIN = 6;

constexpr uint8_t EFUSE_VALVE_O2_PIN = 7;
constexpr uint8_t EFUSE_VALVE_CO2_PIN = 8;
constexpr uint8_t EFUSE_VALVE_AIR_PIN = 9;

constexpr uint8_t EFUSE_FAN_RIGHT_PIN = 10;
constexpr uint8_t EFUSE_FAN_LEFT_PIN = 11;
constexpr uint8_t EFUSE_FAN_CIRCULATION_PIN = 12;
constexpr uint8_t EFUSE_FAN_PCB_PIN = 13;
constexpr uint8_t EFUSE_FAN_24_PIN = 14;
constexpr uint8_t EFUSE_FAN_120_PIN = 15;
constexpr uint8_t EFUSE_FAN_HEATER_PIN = 16;

// SSR RELAY
constexpr int HEATER_PIN = 23;

#endif // PINS_H
