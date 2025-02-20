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
// Relay Valve1(VALVE_1_PIN);
// Relay Valve2(VALVE_2_PIN);
// Relay Valve3(VALVE_3_PIN);
// Relay Valve4(VALVE_4_PIN);
// Relay Valve5(VALVE_5_PIN);
Relay HeaterFan(HEATER_FAN_PIN);
Relay InteriorFan(INTERIOR_FAN_PIN);
// Relay ExteriorFan(EXTERIOR_FAN_PIN);
Relay PatchHeater(PATCH_HEATER_PIN);

int value = 128;
void serialReader();

void setup()
{
    Serial.begin(115200);
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Serial.println("Hello, World!");

    sht40.begin(&Wire);
    pyroscience.begin();
    ApprovPump.begin();
    SensorPump.begin();
    CultureChamberPump1.begin();
    CultureChamberPump2.begin();
    Heater.begin();
    // Valve1.begin();
    // Valve2.begin();
    // Valve3.begin();
    // Valve4.begin();
    // Valve5.begin();
    HeaterFan.begin();
    InteriorFan.begin();
    // ExteriorFan.begin();
    PatchHeater.begin();
}

void loop()
{
    ApprovPump.setSpeed(254);
    Heater.setLevel(value); // temperaturecontroller.getHeaterPower()
    Heater.update();
    serialReader();

    Serial.println(value);
}

void serialReader()
{
    if (Serial.available())
    {
        uint8_t byte = Serial.read();

        if (byte == 'w')
        {
            value++;
        }
        else if (byte == 's')
        {
            value--;
        }
    }
}
