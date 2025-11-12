#include "bioreactor_controller.h"

// Objects declaration
SHT40 sht40[NB_TEMP_SENSOR];
I2CMux muxI2c;
Pyroscience pyroscience;
GMP251 co2Sensor(RS485_RX_PIN, RS485_TX_PIN, RS485_DE_PIN, Serial1);
O2Sensor o2Sensor;
PumpDC approvPump(APPROV_PUMP_PIN_1, APPROV_PUMP_PIN_2);
PumpDC sensorPump(SENSOR_PUMP_PIN_1, SENSOR_PUMP_PIN_2);
PumpStepper cultureChamberPump1(CULTURE_CHAMBER_PUMP_1_PIN_1, CULTURE_CHAMBER_PUMP_1_PIN_2);
PumpStepper cultureChamberPump2(CULTURE_CHAMBER_PUMP_2_PIN_1, CULTURE_CHAMBER_PUMP_2_PIN_2);
SSR_Relay heater(HEATER_PIN);
// Relay valve1(VALVE_1_PIN);
// Relay valve2(VALVE_2_PIN);
// Relay valve3(VALVE_3_PIN);
// Relay valve4(VALVE_4_PIN);
// Relay valve5(VALVE_5_PIN);
Relay O2Valve(O2_VALVE_PIN);
Relay CO2Valve(CO2_VALVE_PIN);
Relay airValve(AIR_VALVE_PIN);
// Relay safetyValve(SAFETY_VALVE_PIN);
Relay heaterFan(HEATER_FAN_PIN);
Relay interiorFan(INTERIOR_FAN_PIN);
// Relay ExteriorFan(EXTERIOR_FAN_PIN);
Relay patchHeater(PATCH_HEATER_PIN);
TemperatureController temperatureController;
PressureChamberController pressureChamber;
VisiFermRS485 DOSensor(RS485_2_RX_PIN, RS485_2_TX_PIN, Serial2);

// Global variables
eBioreactorState bioreactorState = eBioreactorState::TEST;
unsigned long lastTemperatureControllerTime = 0;
unsigned long lastPressureChamberControllerTime = 0;
unsigned long lastPressureChamberControllerTimePrint = 0;
unsigned long lastPrintTime = 0;
uint8_t testState = 0;

/**
 * @brief Call the "begin" of every objects in the bioreactor controller.
 */
void beginBioreactorController()
{
    muxI2c.begin(&Wire);
    for (uint8_t i = 0; i < NB_TEMP_SENSOR; i++)
        sht40[i].begin(&Wire, i, &muxI2c);
    pyroscience.begin(&Serial1);
    co2Sensor.begin();
    o2Sensor.begin();
    DOSensor.begin();

    // Pumps
    approvPump.begin();
    sensorPump.begin();
    cultureChamberPump1.begin();
    cultureChamberPump2.begin();

    // Valves
    // valve1.begin();
    // valve2.begin();
    // valve3.begin();
    // valve4.begin();
    // valve5.begin();
    O2Valve.begin();
    CO2Valve.begin();
    airValve.begin();
    // safetyValve.begin();

    // Fans & Heaters
    heater.begin();
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
 * @param o2ValveState      State of the O2 valve. (OPEN/CLOSE)
 * @param co2ValveState     State of the CO2 valve. (OPEN/CLOSE)
 * @param airValveState     State of the air valve. (OPEN/CLOSE)
 * @param safetyValveState  State of the safety valve. (OPEN/CLOSE)
 */
void setPressureChamberValvesState(bool o2ValveState, bool co2ValveState, bool airValveState, bool safetyValveState)
{
    O2Valve.set(o2ValveState);
    CO2Valve.set(co2ValveState);
    airValve.set(airValveState);
    // safetyValve.set(safetyValveState);
}

/**
 * @brief Set the speed of the pumps.
 *
 * MAX_SPEED for DC pumps is 255 (0-255)
 * MAX_SPEED for stepper pumps is 1000 (0-1000)
 *
 * @param approvPumpSpeed           Speed of the approv pump
 * @param sensorPumpSpeed           Speed of the sensor pump
 * @param cultureChamberPump1Speed  Speed of the culture chamber pump 1
 * @param cultureChamberPump2Speed  Speed of the culture chamber pump 2
 */
void setPumpsSpeed(uint8_t approvPumpSpeed, uint8_t sensorPumpSpeed, uint16_t cultureChamberPump1Speed, uint16_t cultureChamberPump2Speed)
{
    // approvPump.setSpeed(approvPumpSpeed);
    // sensorPump.setSpeed(sensorPumpSpeed);
    // cultureChamberPump1.setSpeed(cultureChamberPump1Speed);
    // cultureChamberPump2.setSpeed(cultureChamberPump2Speed);
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
 * @brief Determine if the pressure chamber needs to be pressurized and regulated.
 * @param state  State of the pressure chamber. (ON/OFF)
 */
void setPressureChamberState(bool state)
{
    pressureChamber.setPressureChamberState(state);
}

/**
 * @brief Update the temperature controller. Must be called in the main loop.
 */
void updateTemperatureController()
{
    heater.update();

    if (millis() - lastTemperatureControllerTime > TEMPERATURE_CONTROLLER_UPDATE_INTERVAL)
    {
        lastTemperatureControllerTime = millis();
        float airTemperature = 0;
        sht40[0].getData(&airTemperature);
        pyroscience.fetchData();
        float waterTemperature = pyroscience.getLastTemperature();

        temperatureController.update(waterTemperature, airTemperature);
    }
}

/**
 * @brief Update the pressure chamber controller. Must be called in the main loop.
 */
void updatePressureChamberController()
{
    if (millis() - lastPressureChamberControllerTime > PRESSURE_CHAMBER_CONTROLLER_UPDATE_INTERVAL)
    {
        lastPressureChamberControllerTime = millis();
        float o2Concentration = o2Sensor.getO2();
        float co2Concentration = co2Sensor.getCO2();
        float pressure = 25 * 6895; // 7.5 psi to Pa // TODO: get this value from the sensor

        pressureChamber.update(o2Concentration, co2Concentration, pressure);
    }

    // // DEBUG: Print every second the O2 and CO2 concentration and the time since last update
    // if (millis() - lastPressureChamberControllerTimePrint > 1000)
    // {
    //     lastPressureChamberControllerTimePrint = millis();
    //     Serial.println(">o2Concentration: " + String(o2Sensor.getO2()));
    //     Serial.println(">co2Concentration: " + String(co2Sensor.getCO2()));
    //     Serial.println(">Time since controller update: " + String(millis() - lastPressureChamberControllerTime));
    // }

    setPressureChamberValvesState(pressureChamber.getValveState(O2),
                                  pressureChamber.getValveState(CO2),
                                  pressureChamber.getValveState(AIR),
                                  pressureChamber.getValveState(SAFETY));

    co2Sensor.update();
}

/**
 * @brief Print all relevant information to the Serial monitor.
 *
 * @note All prints of the code should be done in this function to keep the Serial output organized.
 */
void printBioreactorStateToSerial()
{
    if (millis() - lastPrintTime > PRINT_UPDATE_INTERVAL)
    {
        Serial.println("> Bioreactor State: " + String(static_cast<int>(bioreactorState)));
        Serial.println("> DO Sensor (%sat): " + String(DOSensor.getOxygen()));

        /* Add more prints here*/

        lastPrintTime = millis();
    }
}

/**
 * @brief Update needed sensors. Must be called in the main loop.
 */
void updateSensors()
{
    DOSensor.update();
}
