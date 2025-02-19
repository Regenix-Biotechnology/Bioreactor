#include <main.h>

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

    switch (bioreactorState)
    {
    case eBioreactorState::IDLE:
        closeAllRelay();
        closeAllPumps();
        break;
    case eBioreactorState::APPROV:
        /* code */
        break;
    case eBioreactorState::PREPARE:
        /* code */
        break;
    case eBioreactorState::RUN:
        /* code */
        break;
    case eBioreactorState::TEST:
    {
        // Pour le test complet de fluidique et de chauffage
        HeaterFan.on();
        InteriorFan.on();

        SensorPump.setSpeed(255);

        float airTemperature = sht40.getTemperature();
        float waterTemperature = pyroscience.getTemperature();

        // TemperatureController.update(airTemperature, waterTemperature);
        // float heaterPower = TemperatureController.getHeaterPower();

        break;
    }
    default:
        delay(1000);
        break;
    }
}

void closeAllRelay()
{
    Valve1.off();
    Valve2.off();
    Valve3.off();
    Valve4.off();
    Valve5.off();
    HeaterFan.off();
    InteriorFan.off();
    ExteriorFan.off();
    PatchHeater.off();
    Heater.off();
}

void closeAllPumps()
{
    ApprovPump.stop();
    SensorPump.stop();
    CultureChamberPump1.stop();
    CultureChamberPump2.stop();
}