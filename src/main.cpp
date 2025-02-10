#include <Arduino.h>
#include "SHT40.h"
#include "Pyroscience.h"
#include "Pump.h"
#include "ssr_relay.h"
#include "relay.h"
#include "pins.h"

SHT40 sht40;
Pyroscience pyroscience;
Pump ApprovPump(APPROV_PUMP_PIN_1, APPROV_PUMP_PIN_2);
Pump SensorPump(SENSOR_PUMP_PIN_1, SENSOR_PUMP_PIN_2);
Pump CultureChamberPump1(CULTURE_CHAMBER_PUMP_1_PIN_1, CULTURE_CHAMBER_PUMP_1_PIN_2);
Pump CultureChamberPump2(CULTURE_CHAMBER_PUMP_2_PIN_1, CULTURE_CHAMBER_PUMP_2_PIN_2);
SSR_Relay Heater(HEATER_PIN);

void setup()
{
    Serial.begin(9600);
    Serial.println("Hello, World!");
}

void loop() {}
