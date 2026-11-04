#include "bioreactor_controller.h"
#include <math.h>

// Objects declaration
SHT40 sht40(&Wire);
GMP251 co2Sensor(RS485_RX_PIN, RS485_TX_PIN, RS485_DE_PIN, Serial1);
O2Sensor o2Sensor;
DriveTmc5041 driveStepper1(&SPI, SPI_CS_DRV_1_PIN);
DriveTmc5041 driveStepper3(&SPI, SPI_CS_DRV_3_PIN);
StepperMotor approvPump(&driveStepper3, MOTOR_2, StepperMotor::ML_PER_REV_6_ROLLERS);
StepperMotor cultureChamberPump2(&driveStepper1, MOTOR_2, StepperMotor::ML_PER_REV_4_ROLLERS);
StepperMotor circulationPump(&driveStepper3, MOTOR_1, StepperMotor::ML_PER_REV_6_ROLLERS);
StepperMotor cultureChamberPump1(&driveStepper1, MOTOR_1, StepperMotor::ML_PER_REV_4_ROLLERS);
SSR_Relay heater(HEATER_PIN);
CO2Controller co2Controller;
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
eCleaningCycle cleaningCycle = eCleaningCycle::NONE;
unsigned long lastTemperatureControllerTime = 0;
unsigned long lastPressureChamberControllerTime = 0;
unsigned long lastPressureChamberControllerTimePrint = 0;
unsigned long lastPrintTime = 0;
unsigned long lastLEDUpdateTime = 0;
unsigned long lastCO2UpdateValueTime = millis() + 2200; // 2 secondes (le capteur mesure toutes les 2 secondes)
unsigned long lastCO2UpdateTime = millis() + 2200;      // 2 secondes (le capteur mesure toutes les 2 secondes)
unsigned int TSTABILISATION = 5000;
uint8_t lastLEDState = 0;
float CO2_Value = 0.0f;
unsigned long lastMotorSetSpeedTime = 0;
static float lastRequestedPumpSpeeds[4] = {NAN, NAN, NAN, NAN};
static eSHT40Status airTemperatureStatus = SHT40_STATUS_NOT_INITIALISED;
static unsigned long lastValidAirTemperatureTime = 0;
static const char *lastCleaningErrorCode = "NONE";
static float baseTemperatureReference = 37.0f;
static CultureSetpoints activeCultureSetpoints = {};
static bool cultureSetpointsAvailable = false;
uint8_t testState = 0;
unsigned long stateTimer;

static bool isCultureTemperatureState(eBioreactorState state)
{
    return state == eBioreactorState::APPROV ||
           state == eBioreactorState::PREPARE ||
           state == eBioreactorState::RUN;
}

static bool isValidCultureSetpoints(const CultureSetpoints &setpoints)
{
    return isfinite(setpoints.temperatureMin) &&
           isfinite(setpoints.temperatureMax) &&
           setpoints.temperatureMin <= setpoints.temperatureMax &&
           isfinite(setpoints.circulationPumpSpeed) &&
           setpoints.circulationPumpSpeed >= 0.0f &&
           setpoints.circulationPumpSpeed <= PUMP_MAX_SPEED &&
           isfinite(setpoints.culturePump1Speed) &&
           setpoints.culturePump1Speed >= 0.0f &&
           setpoints.culturePump1Speed <= PUMP_MAX_SPEED &&
           isfinite(setpoints.culturePump2Speed) &&
           setpoints.culturePump2Speed >= 0.0f &&
           setpoints.culturePump2Speed <= PUMP_MAX_SPEED;
}

void applyCultureTemperatureForCurrentState()
{
    float reference = baseTemperatureReference;
    if (cultureSetpointsAvailable && isCultureTemperatureState(bioreactorState))
    {
        reference = activeCultureSetpoints.temperatureMin * 0.5f +
                    activeCultureSetpoints.temperatureMax * 0.5f;
    }
    temperatureController.setReferenceTemperature(reference);
}

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

bool isBioreactorStateSupported(eBioreactorState state)
{
    return state == eBioreactorState::IDLE ||
           state == eBioreactorState::APPROV ||
           state == eBioreactorState::PREPARE ||
           state == eBioreactorState::RUN ||
           state == eBioreactorState::CELL_RETURN ||
           state == eBioreactorState::CLEANING_APPROV ||
           state == eBioreactorState::CLEANING_CIRCULATION ||
           state == eBioreactorState::CLEANING_RETURN ||
           state == eBioreactorState::CLEANING_WAITING_WATER ||
           state == eBioreactorState::CLEANING_WAITING_DETERGENT ||
           state == eBioreactorState::TEST ||
           state == eBioreactorState::TRASH ||
           state == eBioreactorState::MEDIA_CHANGE_TRASH ||
           state == eBioreactorState::MEDIA_CHANGE_APPROV;
}

/**
 * @brief Setter for bioreactor state
 *
 * @param state
 */
void setBioreactorState(uint8_t state_int)
{
    eBioreactorState state = (eBioreactorState)state_int;
    if (!isBioreactorStateSupported(state))
    {
        return;
    }

    const bool stateChanged = state != bioreactorState;

    const bool isCleaningState =
        state == eBioreactorState::CLEANING_APPROV ||
        state == eBioreactorState::CLEANING_CIRCULATION ||
        state == eBioreactorState::CLEANING_RETURN ||
        state == eBioreactorState::CLEANING_WAITING_WATER ||
        state == eBioreactorState::CLEANING_WAITING_DETERGENT;
    if (isCleaningState && cleaningCycle == eCleaningCycle::NONE)
    {
        return;
    }

    if (!isCleaningState)
    {
        const bool wasCleaningState =
            bioreactorState == eBioreactorState::CLEANING_APPROV ||
            bioreactorState == eBioreactorState::CLEANING_CIRCULATION ||
            bioreactorState == eBioreactorState::CLEANING_RETURN ||
            bioreactorState == eBioreactorState::CLEANING_WAITING_WATER ||
            bioreactorState == eBioreactorState::CLEANING_WAITING_DETERGENT;
        if (wasCleaningState)
        {
            setHeatersState(OFF);
        }
        cleaningCycle = eCleaningCycle::NONE;
    }

    bioreactorState = state;
    bioreactorParameter.putShort("state", (int16_t)state);
    stateTimer = millis();
    if (isCleaningState)
    {
        setHeatersState(OFF);
    }
    applyCultureTemperatureForCurrentState();
    if (stateChanged)
        Serial.println("EVENT:STATE=" + String(static_cast<int>(state)));
    return;
}

eCleaningCommandResult setCleaningCyclePhase(eCleaningCycle cycle, eBioreactorState phase)
{
    if (!isValidCleaningOperatorCommand(bioreactorState, cleaningCycle, cycle, phase))
    {
        return eCleaningCommandResult::INVALID_TRANSITION;
    }

    if (cycle == eCleaningCycle::DETERGENT &&
        phase == eBioreactorState::CLEANING_APPROV &&
        !isCleaningTemperatureSafe())
    {
        return eCleaningCommandResult::TEMPERATURE_SENSOR_NOT_READY;
    }

    if (!setCleaningSafeOutputs())
    {
        return eCleaningCommandResult::ACTUATOR_FAILURE;
    }

    lastCleaningErrorCode = "NONE";
    cleaningCycle = cycle;
    setBioreactorState((uint8_t)phase);
    setHeatersState(cycle == eCleaningCycle::DETERGENT);
    return eCleaningCommandResult::ACCEPTED;
}

const char *getLastCleaningErrorCode()
{
    return lastCleaningErrorCode;
}

void advanceCleaningPhase(eBioreactorState phase)
{
    if (!isValidCleaningPhaseTransition(bioreactorState, cleaningCycle, phase))
    {
        failCleaningSequence("INVALID_INTERNAL_TRANSITION");
        return;
    }

    setBioreactorState((uint8_t)phase);
    setHeatersState(cleaningCycle == eCleaningCycle::DETERGENT);
    Serial.println("EVENT:CLEANING:PHASE;CYCLE=" + String(cleaningCycleName(cleaningCycle)) +
                   ";PHASE=" + String(cleaningPhaseName(phase)));
    Serial.println("EVENT:CLEANING:" + String(cleaningCycleName(cleaningCycle)) + ":" +
                   String(cleaningPhaseName(phase)) + "_STARTED");
}

void finishCleaningCycle()
{
    setHeatersState(OFF);
    if (!setCleaningSafeOutputs())
    {
        failCleaningSequence("ACTUATOR_FAILURE_AT_CYCLE_END");
        return;
    }

    if (cleaningCycle == eCleaningCycle::DETERGENT)
    {
        Serial.println("EVENT:CLEANING:CYCLE_END;CYCLE=DETERGENT;PHASE=RETURN");
        setBioreactorState((uint8_t)nextCleaningStateAfterCycle(cleaningCycle));
        Serial.println("EVENT:CLEANING:WAITING_FOR_WATER;CYCLE=DETERGENT;PHASE=WAITING_FOR_WATER");
        Serial.println("EVENT:CLEANING:DETERGENT_COMPLETE");
        Serial.println("EVENT:CLEANING:WAITING_FOR_WATER_CONFIRMATION");
    }
    else if (cleaningCycle == eCleaningCycle::RINSE)
    {
        Serial.println("EVENT:CLEANING:CYCLE_END;CYCLE=RINSE;PHASE=RETURN");
        cleaningCycle = nextCleaningCycle(cleaningCycle);
        setBioreactorState((uint8_t)nextCleaningStateAfterCycle(eCleaningCycle::RINSE));
        setHeatersState(OFF);
        Serial.println("EVENT:CLEANING:CYCLE_START;CYCLE=DRY;PHASE=APPROV");
        Serial.println("EVENT:CLEANING:RINSE_COMPLETE");
        Serial.println("EVENT:CLEANING:DRY_STARTED");
        Serial.println("EVENT:CLEANING:DRY:APPROV_STARTED");
    }
    else if (cleaningCycle == eCleaningCycle::DRY)
    {
        Serial.println("EVENT:CLEANING:CYCLE_END;CYCLE=DRY;PHASE=RETURN");
        setBioreactorState((uint8_t)nextCleaningStateAfterCycle(cleaningCycle));
        Serial.println("EVENT:CLEANING:COMPLETE;CYCLE=DRY;PHASE=COMPLETE");
        Serial.println("EVENT:CLEANING:DRY_COMPLETE");
        Serial.println("EVENT:CLEANING:COMPLETE");
    }
    else
    {
        failCleaningSequence("INVALID_CYCLE_AT_CYCLE_END");
    }
}

bool setCleaningSafeOutputs()
{
    setHeatersState(OFF);
    bool success = setFansState(OFF, OFF, OFF, OFF, OFF, OFF, OFF);
    success = setValvesState(CLOSE, CLOSE, CLOSE) && success;
    success = setPressureChamberValvesState(OFF, OFF, OFF, OFF) && success;
    setPressureChamberState(OFF);
    success = setPumpsSpeed(OFF, OFF, OFF, OFF) && success;
    return success;
}

void failCleaningSequence(const char *reason)
{
    failCleaningSequence(reason, cleaningCycle, bioreactorState);
}

void failCleaningSequence(const char *reason, eCleaningCycle cycle, eBioreactorState phase)
{
    lastCleaningErrorCode = reason;
    bool safeOutputsApplied = setCleaningSafeOutputs();
    setBioreactorState((uint8_t)eBioreactorState::IDLE);
    Serial.println("EVENT:CLEANING:ERROR;CYCLE=" + String(cleaningCycleName(cycle)) +
                   ";PHASE=" + String(cleaningPhaseName(phase)) + ";CODE=" + String(reason));
    Serial.println("ERR:CLEANING:" + String(reason) + ";CYCLE=" + String(cleaningCycleName(cycle)) +
                   ";PHASE=" + String(cleaningPhaseName(phase)));
    if (!safeOutputsApplied)
        Serial.println("EVENT:CLEANING:ERROR;CYCLE=" + String(cleaningCycleName(cycle)) +
                       ";PHASE=" + String(cleaningPhaseName(phase)) +
                       ";CODE=SAFE_OUTPUT_COMMAND_FAILED");
}

void reportCleaningStartFailure(
    const char *reason,
    eCleaningCycle cycle,
    eBioreactorState phase)
{
    lastCleaningErrorCode = reason;
    const bool safeOutputsApplied = setCleaningSafeOutputs();
    Serial.println("EVENT:CLEANING:ERROR;CYCLE=" + String(cleaningCycleName(cycle)) +
                   ";PHASE=" + String(cleaningPhaseName(phase)) + ";CODE=" + String(reason));
    if (!safeOutputsApplied)
    {
        lastCleaningErrorCode = "SAFE_OUTPUT_COMMAND_FAILED";
        Serial.println("EVENT:CLEANING:ERROR;CYCLE=" + String(cleaningCycleName(cycle)) +
                       ";PHASE=" + String(cleaningPhaseName(phase)) +
                       ";CODE=SAFE_OUTPUT_COMMAND_FAILED");
    }
}

bool stopCleaningSequence()
{
    const bool cleaningActive = canStopCleaningSequence(bioreactorState);
    if (!cleaningActive)
        return false;

    const eCleaningCycle stoppedCycle = cleaningCycle;
    const eBioreactorState stoppedPhase = bioreactorState;
    bool safeOutputsApplied = setCleaningSafeOutputs();
    setBioreactorState((uint8_t)eBioreactorState::IDLE);
    if (safeOutputsApplied)
    {
        Serial.println("EVENT:CLEANING:STOPPED;CYCLE=" + String(cleaningCycleName(stoppedCycle)) +
                       ";PHASE=" + String(cleaningPhaseName(stoppedPhase)));
        Serial.println("EVENT:CLEANING:STOPPED");
    }
    else
    {
        lastCleaningErrorCode = "SAFE_OUTPUT_COMMAND_FAILED";
        Serial.println("EVENT:CLEANING:ERROR;CYCLE=" + String(cleaningCycleName(stoppedCycle)) +
                       ";PHASE=" + String(cleaningPhaseName(stoppedPhase)) +
                       ";CODE=SAFE_OUTPUT_COMMAND_FAILED");
    }
    return safeOutputsApplied;
}

/**
 * @brief Get all the culture parameter from memory
 */
void beginBioreactorPreferences()
{
    bioreactorParameter.begin("bioreactor");

    baseTemperatureReference = bioreactorParameter.getFloat("temperature", 37.0f);
    float ph = bioreactorParameter.getFloat("ph", 7.0);
    float oxyDissous = bioreactorParameter.getFloat("do", 100.0);
    float co2 = bioreactorParameter.getFloat("CO2", 50000.0);
    float dioxyg = bioreactorParameter.getFloat("O2", 85.0);
    int16_t storedState = bioreactorParameter.getShort("state", (int16_t)eBioreactorState::IDLE);
    eBioreactorState state = eBioreactorState::IDLE;
    if (storedState >= 0 && isBioreactorStateSupported((eBioreactorState)storedState))
    {
        state = (eBioreactorState)storedState;
    }

    state = getSafeStartupState(state);

    temperatureController.setReferenceTemperature(baseTemperatureReference);
    pressureChamber.setReferenceLevel(CO2, co2);
    pressureChamber.setReferenceLevel(O2, dioxyg);
    co2Controller.setReferenceLevel(4.5f);

    cultureSetpointsAvailable = false;
    if (bioreactorParameter.isKey("cultureCfg"))
    {
        const size_t configSize = bioreactorParameter.getBytesLength("cultureCfg");
        CultureSetpoints storedSetpoints;
        if (configSize == sizeof(storedSetpoints) &&
            bioreactorParameter.getBytes("cultureCfg", &storedSetpoints, sizeof(storedSetpoints)) == sizeof(storedSetpoints) &&
            isValidCultureSetpoints(storedSetpoints))
        {
            activeCultureSetpoints = storedSetpoints;
            cultureSetpointsAvailable = true;
        }
        else
        {
            Serial.println("ERR:CULTURE:STORED_CONFIG_INVALID");
        }
    }

    // ph
    // oxy_dissous
    bioreactorState = state;
    cleaningCycle = eCleaningCycle::NONE;
    applyCultureTemperatureForCurrentState();
    if (storedState != (int16_t)state)
    {
        bioreactorParameter.putShort("state", (int16_t)state);
    }
    // pump
}

eCultureSetpointResult stageCultureSetpoints(const CultureSetpoints &setpoints)
{
    if (bioreactorState != eBioreactorState::IDLE)
        return eCultureSetpointResult::INVALID_STATE;
    if (!isValidCultureSetpoints(setpoints))
        return eCultureSetpointResult::INVALID_VALUES;

    if (bioreactorParameter.putBytes("cultureCfg", &setpoints, sizeof(setpoints)) != sizeof(setpoints))
        return eCultureSetpointResult::STORAGE_FAILURE;

    activeCultureSetpoints = setpoints;
    cultureSetpointsAvailable = true;
    applyCultureTemperatureForCurrentState();
    return eCultureSetpointResult::ACCEPTED;
}

bool setBaseTemperatureReference(float temperature)
{
    if (!isfinite(temperature) ||
        bioreactorParameter.putFloat("temperature", temperature) != sizeof(float))
        return false;

    baseTemperatureReference = temperature;
    applyCultureTemperatureForCurrentState();
    return true;
}

bool getRunCulturePumpSpeeds(
    float &circulationSpeed,
    float &culturePump1Speed,
    float &culturePump2Speed)
{
    if (bioreactorState != eBioreactorState::RUN || !cultureSetpointsAvailable)
        return false;

    circulationSpeed = activeCultureSetpoints.circulationPumpSpeed;
    culturePump1Speed = activeCultureSetpoints.culturePump1Speed;
    culturePump2Speed = activeCultureSetpoints.culturePump2Speed;
    return true;
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
bool setFansState(bool heaterFanState, bool circulationFanState, bool rightFanState, bool leftFanState, bool pcbFanState, bool lowVoltFanState, bool highvoltFanState)
{
    bool success = ioExpander.setEfuse(EFUSE_FAN_HEATER_INDEX, heaterFanState);
    success = ioExpander.setEfuse(EFUSE_FAN_CIRCULATION_INDEX, circulationFanState) && success;
    success = ioExpander.setEfuse(EFUSE_FAN_RIGHT_INDEX, rightFanState) && success;
    success = ioExpander.setEfuse(EFUSE_FAN_LEFT_INDEX, leftFanState) && success;
    success = ioExpander.setEfuse(EFUSE_FAN_PCB_INDEX, pcbFanState) && success;
    success = ioExpander.setEfuse(EFUSE_FAN_LOW_VOLT_INDEX, lowVoltFanState) && success;
    success = ioExpander.setEfuse(EFUSE_FAN_HIGH_VOLT_INDEX, highvoltFanState) && success;
    return success;
}

/**
 * @brief Set the state of the valves.
 * @param valveApprovState        State of the supply valve. (OPEN/CLOSE)
 * @param valveCirculationState   State of the circulation valve. (OPEN/CLOSE)
 * @param valveCleaningState      State of the cleaning solution valve. (OPEN/CLOSE)
 */
bool setValvesState(bool valveSupplyState, bool valveCirculationState, bool valveCleaningState)
{
    const bool supplyOk = ioExpander.setEfuse(EFUSE_VALVE_SUPPLY_INDEX, valveSupplyState);
    if (!supplyOk)
    {
        Serial.print("ERR:IO:OUTPUT_FAILURE;OUTPUT=SUPPLY_VALVE;CHANNEL=");
        Serial.println(EFUSE_VALVE_SUPPLY_INDEX);
    }

    const bool circulationOk = ioExpander.setEfuse(EFUSE_VALVE_CIRCULATION_INDEX, valveCirculationState);
    if (!circulationOk)
    {
        Serial.print("ERR:IO:OUTPUT_FAILURE;OUTPUT=CIRCULATION_VALVE;CHANNEL=");
        Serial.println(EFUSE_VALVE_CIRCULATION_INDEX);
    }

    const bool returnOk = ioExpander.setEfuse(EFUSE_VALVE_RETURN_INDEX, valveCleaningState);
    if (!returnOk)
    {
        Serial.print("ERR:IO:OUTPUT_FAILURE;OUTPUT=RETURN_VALVE;CHANNEL=");
        Serial.println(EFUSE_VALVE_RETURN_INDEX);
    }

    return supplyOk && circulationOk && returnOk;
}

/**
 * @brief Set the state of the pressure chamber valves.
 * @param o2ValveState      State of the O2 valve. (OPEN/CLOSE)
 * @param co2ValveState     State of the CO2 valve. (OPEN/CLOSE)
 * @param airValveState     State of the air valve. (OPEN/CLOSE)
 */
bool setPressureChamberValvesState(bool o2ValveState, bool co2ValveState, bool airValveState, bool co2AirValveState)
{
    bool success = ioExpander.setEfuse(EFUSE_VALVE_O2_INDEX, o2ValveState);
    success = ioExpander.setEfuse(EFUSE_VALVE_CO2_INDEX, co2ValveState) && success;
    success = ioExpander.setEfuse(EFUSE_VALVE_AIR_INDEX, airValveState) && success;
    success = ioExpander.setEfuse(EFUSE_VALVE_CO2_ATM_INDEX, co2AirValveState) && success;
    return success;
}

/**
 * @brief Set the speed of the pumps And the number iof micro-steps for each pump.
 * Speed is in float ml/min and +/- for direction
 * @param approvPumpSpeed           Speed of the approv pump
 * @param circulationPumpSpeed           Speed of the sensor pump
 * @param cultureChamberPump1Speed  Speed of the culture chamber pump 1
 * @param cultureChamberPump2Speed  Speed of the culture chamber pump 2
 */
bool setPumpsSpeed(float approvPumpSpeed, float circulationPumpSpeed, float cultureChamberPump1Speed, float cultureChamberPump2Speed)
{
    const float requestedSpeeds[4] = {
        approvPumpSpeed,
        circulationPumpSpeed,
        cultureChamberPump1Speed,
        cultureChamberPump2Speed};
    bool speedChanged = false;
    for (uint8_t i = 0; i < 4; ++i)
    {
        if (requestedSpeeds[i] != lastRequestedPumpSpeeds[i])
        {
            speedChanged = true;
            break;
        }
    }

    if (speedChanged || millis() - lastMotorSetSpeedTime > MOTOR_SET_SPEED_MSG_INTERVAL)
    {
        eMotorStatus approvStatus = approvPump.setSpeed(approvPumpSpeed, 128);
        eMotorStatus circulationStatus = circulationPump.setSpeed(circulationPumpSpeed, 128);
        eMotorStatus culturePump1Status = cultureChamberPump1.setSpeed(cultureChamberPump1Speed, 128);
        eMotorStatus culturePump2Status = cultureChamberPump2.setSpeed(cultureChamberPump2Speed, 128);
        lastMotorSetSpeedTime = millis();
        bool success = approvStatus == MOTOR_STATUS_OK &&
                       circulationStatus == MOTOR_STATUS_OK &&
                       culturePump1Status == MOTOR_STATUS_OK &&
                       culturePump2Status == MOTOR_STATUS_OK;
        if (success)
        {
            for (uint8_t i = 0; i < 4; ++i)
                lastRequestedPumpSpeeds[i] = requestedSpeeds[i];
        }
        return success;
    }
    return true;
}

/**
 * @brief Set the state of the heaters.
 * @param heaterState       State of the heater. (0-100%)
 *
 */
void setHeatersState(bool heaterState)
{
    if (heaterState && isHeaterPermitted(bioreactorState, cleaningCycle, isCleaningTemperatureSafe()))
        heater.setLevel(temperatureController.getHeaterPower());
    else
        heater.setLevel(OFF);
}

bool isCleaningTemperatureSafe()
{
    const unsigned long now = millis();
    return tempSensor.getStatus() == ATLAS_STATUS_OK &&
           tempSensor.getAgeMs() <= 3 * TEMPERATURE_CONTROLLER_UPDATE_INTERVAL &&
           isfinite(tempSensor.getTemperatureC()) &&
           airTemperatureStatus == SHT40_STATUS_OK &&
           lastValidAirTemperatureTime != 0 &&
           now - lastValidAirTemperatureTime <= 3 * TEMPERATURE_CONTROLLER_UPDATE_INTERVAL &&
           isfinite(sht40.getLastTemperature());
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
    if (millis() - lastTemperatureControllerTime > TEMPERATURE_CONTROLLER_UPDATE_INTERVAL)
    {
        lastTemperatureControllerTime = millis();
        float airTemperature = 0;
        airTemperatureStatus = sht40.getData(&airTemperature);
        if (airTemperatureStatus == SHT40_STATUS_OK)
            lastValidAirTemperatureTime = millis();
        float waterTemperature = tempSensor.getTemperatureC();

        if (airTemperatureStatus == SHT40_STATUS_OK &&
            tempSensor.getStatus() == ATLAS_STATUS_OK &&
            tempSensor.getAgeMs() <= 3 * TEMPERATURE_CONTROLLER_UPDATE_INTERVAL &&
            isfinite(airTemperature) &&
            isfinite(waterTemperature))
        {
            temperatureController.update(waterTemperature, airTemperature);
            setHeatersState(ON);
        }
        else
        {
            temperatureController.reset();
            setHeatersState(OFF);
        }
    }

    heater.update();
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
                                  co2Controller.getValveState());

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
        Serial.println(">  Water Temperature setpoint (°C): " + String(temperatureController.getReferenceTemperature()));
        Serial.println("> CO2 concentration (%) Bioreactor air: " + String(CO2_Value));

        /* Add more prints here*/
        // Serial.println("> TEST status: " + String(getStatusSTATETEST()));
        // Serial.println("> TEST Stabilisation status: " + String(getStatusSTAB_STATE_TEST()));

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

/**
 * @brief Update the CO2 controller. Must be called in the main loop.
 */
void updateCO2Controller()
{
    if (millis() - lastCO2UpdateValueTime > 2000)
    {
        CO2_Value = ledI2C.getCO2Value();
        lastCO2UpdateValueTime = millis();
        // Serial.println(CO2_Value);
    }
    co2Controller.update(CO2_Value);
}