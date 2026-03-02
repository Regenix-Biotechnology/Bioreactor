#include "bioreactor_controller.h"

// Objects declaration
SHT40 sht40(&Wire);
GMP251 co2Sensor(RS485_RX_PIN, RS485_TX_PIN, RS485_DE_PIN, Serial1);
O2Sensor o2Sensor;
DriveTmc5041 driveStepper1(&SPI, SPI_CS_DRV_1_PIN);
DriveTmc5041 driveStepper3(&SPI, SPI_CS_DRV_3_PIN);
StepperMotor approvPump(&driveStepper1, MOTOR_1);
StepperMotor cultureChamberPump2(&driveStepper1, MOTOR_2);
StepperMotor cultureChamberPump1(&driveStepper3, MOTOR_1);
StepperMotor circulationPump(&driveStepper3, MOTOR_2);
SSR_Relay heater(HEATER_PIN);
IOExpander ioExpander(&Wire);
TemperatureController temperatureController;
PressureChamberController pressureChamber;
VisiFermRS485 dissolvedOxygenSensor(RS485_2_RX_PIN, RS485_2_TX_PIN, Serial2);
AtlasPHSensor pHSensor(&Wire);
AtlasTempSensor tempSensor(&Wire);
LimitSwitch limitSwitch(LIMIT_SWITCH_PIN);
LedI2C ledI2C(&Wire);
Preferences bioreactorParameter;

// Global variables
eBioreactorState bioreactorState = eBioreactorState::TEST;
unsigned long lastTemperatureControllerTime = 0;
unsigned long lastPressureChamberControllerTime = 0;
unsigned long lastPressureChamberControllerTimePrint = 0;
unsigned long lastPrintTime = 0;
unsigned long lastLEDUpdateTime = 0;
uint8_t lastLEDState = 0;
unsigned long lastMotorSetSpeedTime = 0;
uint8_t testState = 0;
unsigned long stateTimer;

/**
 * @brief Call the "begin" of every objects in the bioreactor controller.
 */
void beginBioreactorController()
{
    ioExpander.begin(); // Initialize the IO Expander first to ensure a short delay before turning the valves and fans off
    sht40.begin();
    co2Sensor.begin();
    o2Sensor.begin();
    dissolvedOxygenSensor.begin();
    pHSensor.begin();
    tempSensor.begin();
    heater.begin();
    limitSwitch.begin();

    // Pumps
    SPI.begin();
    driveStepper1.begin();
    driveStepper3.begin();
    approvPump.begin();
    circulationPump.begin();
    cultureChamberPump1.begin();
    cultureChamberPump2.begin();
    beginBioreactorPreferences();
}

/**
 * @brief Setter for bioreactor state
 *
 * @param state
 */
void setBioreactorState(uint8_t state_int)
{
    eBioreactorState state = (eBioreactorState)state_int;
    if (state >= eBioreactorState::MAX_STATE)
    {
        return;
    }

    bioreactorState = state;
    bioreactorParameter.putShort("state", (int16_t)state);
    stateTimer = millis();
    return;
}

/**
 * @brief Get all the culture parameter from memory
 */
void beginBioreactorPreferences()
{
    bioreactorParameter.begin("bioreactor");

    float temperature = bioreactorParameter.getFloat("temperature", 37.0);
    float ph = bioreactorParameter.getFloat("ph", 7.0);
    float oxyDissous = bioreactorParameter.getFloat("do", 100.0);
    float co2 = bioreactorParameter.getFloat("CO2", 50000.0);
    float dioxyg = bioreactorParameter.getFloat("O2", 85.0);
    eBioreactorState state = (eBioreactorState)bioreactorParameter.getShort("state", (int16_t)eBioreactorState::IDLE);

    temperatureController.setReferenceTemperature(temperature);
    pressureChamber.setReferenceLevel(CO2, co2);
    pressureChamber.setReferenceLevel(O2, dioxyg);

    // ph
    // oxy_dissous
    bioreactorState = state;
    // pump
}

/**
 * @brief Set the state of the fans.
 * @param heaterFanState        State of the heater fan (ON/OFF)
 * @param circulationFanState   State of the interior circulation fan (ON/OFF) that circulates air inside the bioreactor
 * @param rightFanState         State of the right fan (ON/OFF) to cool the right side box containing the stepper pumps
 * @param leftFanState          State of the left fan (ON/OFF) to cool the left side box containing the stepper motors pumps
 * @param pcbFanState           State of the PCB fan (ON/OFF) to cool the PCB board
 * @param lowVoltFanState       State of the 24V fan (ON/OFF) to cool the low voltage power supply side of the electrical panel
 * @param highVoltFanState      State of the 120V fan (ON/OFF) to cool the high voltage power supply side of the electrical panel
 */
void setFansState(bool heaterFanState, bool circulationFanState, bool rightFanState, bool leftFanState, bool pcbFanState, bool lowVoltFanState, bool highVoltFanState)
{
    ioExpander.setEfuse(EFUSE_FAN_HEATER_INDEX, heaterFanState);
    ioExpander.setEfuse(EFUSE_FAN_CIRCULATION_INDEX, circulationFanState);
    ioExpander.setEfuse(EFUSE_FAN_RIGHT_INDEX, rightFanState);
    ioExpander.setEfuse(EFUSE_FAN_LEFT_INDEX, leftFanState);
    ioExpander.setEfuse(EFUSE_FAN_PCB_INDEX, pcbFanState);
    ioExpander.setEfuse(EFUSE_FAN_LOW_VOLT_INDEX, lowVoltFanState);
    ioExpander.setEfuse(EFUSE_FAN_HIGH_VOLT_INDEX, highVoltFanState);
}

/**
 * @brief Set the state of the valves.
 * @param valveApprovState        State of the supply valve. (OPEN/CLOSE)
 * @param valveCirculationState   State of the circulation valve. (OPEN/CLOSE)
 * @param valveCleaningState      State of the cleaning solution valve. (OPEN/CLOSE)
 */
void setValvesState(bool valveSupplyState, bool valveCirculationState, bool valveCleaningState)
{
    ioExpander.setEfuse(EFUSE_VALVE_SUPPLY_INDEX, valveSupplyState);
    ioExpander.setEfuse(EFUSE_VALVE_CIRCULATION_INDEX, valveCirculationState);
    ioExpander.setEfuse(EFUSE_VALVE_RETURN_INDEX, valveCleaningState);
}

/**
 * @brief Set the state of the pressure chamber valves.
 * @param o2ValveState      State of the O2 valve. (OPEN/CLOSE)
 * @param co2ValveState     State of the CO2 valve. (OPEN/CLOSE)
 * @param airValveState     State of the air valve. (OPEN/CLOSE)
 */
void setPressureChamberValvesState(bool o2ValveState, bool co2ValveState, bool airValveState)
{
    ioExpander.setEfuse(EFUSE_VALVE_O2_INDEX, o2ValveState);
    ioExpander.setEfuse(EFUSE_VALVE_CO2_INDEX, co2ValveState);
    ioExpander.setEfuse(EFUSE_VALVE_AIR_INDEX, airValveState);
}

/**
 * @brief Set the speed of the pumps.
 *
 * Speed is in float ml/min and +/- for direction
 *
 * @param approvPumpSpeed           Speed of the approv pump
 * @param circulationPumpSpeed           Speed of the sensor pump
 * @param cultureChamberPump1Speed  Speed of the culture chamber pump 1
 * @param cultureChamberPump2Speed  Speed of the culture chamber pump 2
 */
void setPumpsSpeed(float approvPumpSpeed, float circulationPumpSpeed, float cultureChamberPump1Speed, float cultureChamberPump2Speed)
{
    if (millis() - lastMotorSetSpeedTime > MOTOR_SET_SPEED_MSG_INTERVAL)
    {
        approvPump.setSpeed(approvPumpSpeed);
        circulationPump.setSpeed(circulationPumpSpeed);
        cultureChamberPump1.setSpeed(cultureChamberPump1Speed);
        cultureChamberPump2.setSpeed(cultureChamberPump2Speed);

        lastMotorSetSpeedTime = millis();
    }
}

/**
 * @brief Set the state of the heaters.
 * @param heaterState       State of the heater. (0-100%)
 *
 */
void setHeatersState(bool heaterState)
{
    if (heaterState)
        heater.setLevel(temperatureController.getHeaterPower());
    else
        heater.setLevel(OFF);
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
        sht40.getData(&airTemperature);
        float waterTemperature = tempSensor.getTemperatureC();

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
        Serial.println("> DO Sensor (%sat): " + String(dissolvedOxygenSensor.getOxygen()));
        Serial.println("> pH Sensor (pH): " + String(pHSensor.getPH()));
        Serial.println("> Water Temperature (°C): " + String(tempSensor.getTemperatureC()));
        Serial.println("> Air Temperature (°C): " + String(sht40.getLastTemperature()));
        Serial.println("> Air Humidity (%RH): " + String(sht40.getLastHumidity()));
        Serial.println("> Heater Power (%): " + String(temperatureController.getHeaterPower()));
        Serial.println("> CO2 Concentration (ppm): " + String(co2Sensor.getCO2()));
        Serial.println("> O2 Concentration (%): " + String(o2Sensor.getO2()));
        Serial.println("> O2 status: " + String(o2Sensor.getStatus()));
        Serial.println("> PH status: " + String(pHSensor.getStatus()));
        Serial.println("> Temperature culture status: " + String(tempSensor.getStatus()));
        Serial.println("> CO2 status: " + String(co2Sensor.getStatus()));
        Serial.println("> DO status: " + String(dissolvedOxygenSensor.getStatus()));

        /* Add more prints here*/

        Serial.println("");
        lastPrintTime = millis();
    }
}

/**
 * @brief Update needed sensors. Must be called in the main loop.
 */
void updateSensors()
{
    dissolvedOxygenSensor.update();
    pHSensor.update();
    tempSensor.update();
}

/**
 * @brief Update the LED state. Must be called in the main loop.
 *
 * Currently, the LED only indicates if the door is open or closed but more states can be added later (ex: error state).
 */
void updateLEDState()
{
    bool isDoorOpen = limitSwitch.getDoorState();
    eLedState ledState = isDoorOpen ? LED_STATE_DOOR_OPEN : LED_STATE_IDLE;

    // Update the LED at each change for fast response and at every LED_UPDATE_INTERVAL to ensure periodic updates
    if (ledState != lastLEDState || (millis() - lastLEDUpdateTime) > LED_UPDATE_INTERVAL)
    {
        lastLEDState = ledState;
        ledI2C.sendState(ledState);
        lastLEDUpdateTime = millis();
    }
}
