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
IOExpander ioExpander(&Wire);
TemperatureController temperatureController;
PressureChamberController pressureChamber;

// Global variables
eBioreactorState bioreactorState = eBioreactorState::TEST;
unsigned long lastTemperatureControllerTime = 0;
unsigned long lastPressureChamberControllerTime = 0;
unsigned long lastPressureChamberControllerTimePrint = 0;
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

    // Pumps
    approvPump.begin();
    sensorPump.begin();
    cultureChamberPump1.begin();
    cultureChamberPump2.begin();

    ioExpander.begin();
}

/**
 * @brief Set the state of the fans.
 * @param heaterFanState        State of the heater fan. (ON/OFF)
 * @param circulationFanState   State of the interior fan. (ON/OFF)
 * @param rightFanState         State of the exterior fan. (ON/OFF)
 * @param leftFanState          State of the exterior fan. (ON/OFF)
 * @param pcbFanState           State of the PCB fan. (ON/OFF)
 * @param lowVoltFanState       State of the 24V fan. (ON/OFF)
 * @param highvoltFanState      State of the 120V fan. (ON/OFF)
 */
void setFansState(bool heaterFanState, bool circulationFanState, bool rightFanState, bool leftFanState, bool pcbFanState, bool lowVoltFanState, bool highvoltFanState)
{
    ioExpander.setEfuse(EFUSE_FAN_HEATER_PIN, heaterFanState);
    ioExpander.setEfuse(EFUSE_FAN_CIRCULATION_PIN, circulationFanState);
    ioExpander.setEfuse(EFUSE_FAN_RIGHT_PIN, rightFanState);
    ioExpander.setEfuse(EFUSE_FAN_LEFT_PIN, leftFanState);
    ioExpander.setEfuse(EFUSE_FAN_PCB_PIN, pcbFanState);
    ioExpander.setEfuse(EFUSE_FAN_24_PIN, lowVoltFanState);
    ioExpander.setEfuse(EFUSE_FAN_120_PIN, highvoltFanState);
}

/**
 * @brief Set the state of the valves.
 * @param valveApprovState        State of the supply valve. (OPEN/CLOSE)
 * @param valveCirculationState   State of the circulation valve. (OPEN/CLOSE)
 * @param valveReturnState        State of the Return valve. (OPEN/CLOSE)
 */
void setValvesState(bool valveSupplyState, bool valveCirculationState, bool valveReturnState)
{
    ioExpander.setEfuse(EFUSE_VALVE_APPROV_PIN, valveSupplyState);
    ioExpander.setEfuse(EFUSE_VALVE_CIRCULATION_PIN, valveCirculationState);
    ioExpander.setEfuse(EFUSE_VALVE_RETOUR_PIN, valveReturnState);
}

/**
 * @brief Set the state of the pressure chamber valves.
 * @param o2ValveState      State of the O2 valve. (OPEN/CLOSE)
 * @param co2ValveState     State of the CO2 valve. (OPEN/CLOSE)
 * @param airValveState     State of the air valve. (OPEN/CLOSE)
 */
void setPressureChamberValvesState(bool o2ValveState, bool co2ValveState, bool airValveState)
{
    ioExpander.setEfuse(EFUSE_VALVE_O2_PIN, o2ValveState);
    ioExpander.setEfuse(EFUSE_VALVE_CO2_PIN, co2ValveState);
    ioExpander.setEfuse(EFUSE_VALVE_AIR_PIN, airValveState);
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
 *
 */
void setHeatersState(float heaterState)
{
    heater.setLevel(heaterState);
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
                                  pressureChamber.getValveState(AIR));

    co2Sensor.update();
}
