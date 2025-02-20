#include "bioreactor_controller.h"

// Objects declaration
SHT40 sht40;
Pyroscience pyroscience;
Pump approvPump(APPROV_PUMP_PIN_1, APPROV_PUMP_PIN_2);
Pump sensorPump(SENSOR_PUMP_PIN_1, SENSOR_PUMP_PIN_2);
Pump cultureChamberPump1(CULTURE_CHAMBER_PUMP_1_PIN_1, CULTURE_CHAMBER_PUMP_1_PIN_2);
Pump cultureChamberPump2(CULTURE_CHAMBER_PUMP_2_PIN_1, CULTURE_CHAMBER_PUMP_2_PIN_2);
SSR_Relay heater(HEATER_PIN);
// Relay valve1(VALVE_1_PIN);
// Relay valve2(VALVE_2_PIN);
// Relay valve3(VALVE_3_PIN);
// Relay valve4(VALVE_4_PIN);
// Relay valve5(VALVE_5_PIN);
// Relay O2Valve(O2_VALVE_PIN);
// Relay CO2Valve(CO2_VALVE_PIN);
// Relay airValve(AIR_VALVE_PIN);
// Relay safetyValve(SAFETY_VALVE_PIN);
Relay heaterFan(HEATER_FAN_PIN);
Relay interiorFan(INTERIOR_FAN_PIN);
// Relay ExteriorFan(EXTERIOR_FAN_PIN);
Relay patchHeater(PATCH_HEATER_PIN);
TemperatureController temperatureController;

// Global variables
eBioreactorState bioreactorState = eBioreactorState::TEST;
unsigned long lastTemperatureControllerTime = 0;
uint8_t testState = 0;

/**
 * @brief Call the "begin" of every objects in the bioreactor controller.
 */
void beginBioreactorController()
{
    sht40.begin(&Wire);
    pyroscience.begin(&Serial1);
    approvPump.begin();
    sensorPump.begin();
    cultureChamberPump1.begin();
    cultureChamberPump2.begin();
    heater.begin();
    // valve1.begin();
    // valve2.begin();
    // valve3.begin();
    // valve4.begin();
    // valve5.begin();
    // O2Valve.begin();
    // CO2Valve.begin();
    // airValve.begin();
    // safetyValve.begin();
    heaterFan.begin();
    interiorFan.begin();
    // exteriorFan.begin();
    patchHeater.begin();
}

/**
 * @brief Set the state of the fans.
 * @param heaterFanState    State of the heater fan. (ON/OFF)
 * @param interiorFanState  State of the interior fan. (ON/OFF)
 * @param exteriorFanState  State of the exterior fan. (ON/OFF)
 */
void setFansState(bool heaterFanState, bool interiorFanState, bool exteriorFanState)
{
    heaterFan.set(heaterFanState);
    interiorFan.set(interiorFanState);
    // exteriorFan.set(exteriorFanState);
}

/**
 * @brief Set the state of the valves.
 * @param valve1State   State of the valve 1. (OPEN/CLOSE)
 * @param valve2State   State of the valve 2. (OPEN/CLOSE)
 * @param valve3State   State of the valve 3. (OPEN/CLOSE)
 * @param valve4State   State of the valve 4. (OPEN/CLOSE)
 * @param valve5State   State of the valve 5. (OPEN/CLOSE)
 */
void setValvesState(bool valve1State, bool valve2State, bool valve3State, bool valve4State, bool valve5State)
{
    // valve1.set(valve1State);
    // valve2.set(valve2State);
    // valve3.set(valve3State);
    // valve4.set(valve4State);
    // valve5.set(valve5State);
}

/**
 * @brief Set the state of the pressure chamber valves.
 * @param O2ValveState      State of the O2 valve. (OPEN/CLOSE)
 * @param CO2ValveState     State of the CO2 valve. (OPEN/CLOSE)
 * @param airValveState     State of the air valve. (OPEN/CLOSE)
 * @param safetyValveState  State of the safety valve. (OPEN/CLOSE)
 */
void setPressureChamberValvesState(bool O2ValveState, bool CO2ValveState, bool airValveState, bool safetyValveState)
{
    // O2Valve.set(O2ValveState);
    // CO2Valve.set(CO2ValveState);
    // airValve.set(airValveState);
    // safetyValve.set(safetyValveState);
}

/**
 * @brief Set the speed of the pumps.
 * @param approvPumpSpeed           Speed of the approv pump. (0-255)
 * @param sensorPumpSpeed           Speed of the sensor pump. (0-255)
 * @param cultureChamberPump1Speed  Speed of the culture chamber pump 1. (0-255)
 * @param cultureChamberPump2Speed  Speed of the culture chamber pump 2. (0-255)
 */
void setPumpsSpeed(uint8_t approvPumpSpeed, uint8_t sensorPumpSpeed, uint8_t cultureChamberPump1Speed, uint8_t cultureChamberPump2Speed)
{
    approvPump.setSpeed(approvPumpSpeed);
    sensorPump.setSpeed(sensorPumpSpeed);
    cultureChamberPump1.setSpeed(cultureChamberPump1Speed);
    cultureChamberPump2.setSpeed(cultureChamberPump2Speed);
}

/**
 * @brief Set the state of the heaters.
 * @param heaterState       State of the heater. (0-100%)
 * @param patchHeaterState  State of the patch heater. (ON/OFF)
 */
void setHeatersState(float heaterState, bool patchHeaterState)
{
    heater.setLevel(heaterState);
    patchHeater.set(patchHeaterState);
}

/**
 * @brief Update the temperature controller. Must be called in the main loop.
 */
void updateTemperatureController()
{
    if (millis() - lastTemperatureControllerTime > TEMPERATURE_CONTROLLER_UPDATE_INTERVAL)
    {
        lastTemperatureControllerTime = millis();
        float airTemperature = 0;
        sht40.getData(&airTemperature);
        pyroscience.fetchData();
        float waterTemperature = pyroscience.getLastTemperature();

        temperatureController.update(waterTemperature, airTemperature);
    }
}
