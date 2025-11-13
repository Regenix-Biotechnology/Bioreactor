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
constexpr uint8_t EFUSE_VALVE_SUPPLY_INDEX = 0;
constexpr uint8_t EFUSE_VALVE_CIRCULATION_INDEX = 1;
constexpr uint8_t EFUSE_VALVE_RETURN_INDEX = 2;

constexpr uint8_t EFUSE_VALVE_O2_INDEX = 3;
constexpr uint8_t EFUSE_VALVE_CO2_INDEX = 4;
constexpr uint8_t EFUSE_VALVE_AIR_INDEX = 5;

constexpr uint8_t EFUSE_FAN_CIRCULATION_INDEX = 6;
constexpr uint8_t EFUSE_FAN_PCB_INDEX = 7;
constexpr uint8_t EFUSE_FAN_HEATER_INDEX = 8;
constexpr uint8_t EFUSE_FAN_RIGHT_INDEX = 16;
constexpr uint8_t EFUSE_FAN_LEFT_INDEX = 17;
constexpr uint8_t EFUSE_FAN_LOW_VOLT_INDEX = 18;
constexpr uint8_t EFUSE_FAN_HIGH_VOLT_INDEX = 19;

// SSR RELAY
constexpr int HEATER_PIN = 23;

#endif // PINS_H
