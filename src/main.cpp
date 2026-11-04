#include "main.h"

float puissnace_pompes;

static bool applyCleaningPhaseOutputs()
{
    bool success = true;
    const bool fansOk = setFansState(OFF, OFF, ON, ON, ON, ON, ON);
    if (!fansOk)
        Serial.println("ERR:CLEANING:OUTPUT_FAILURE;OUTPUT=FANS");
    success = fansOk && success;

    bool pumpsOk = false;
    bool valvesOk = false;
    if (bioreactorState == eBioreactorState::CLEANING_APPROV)
    {
        pumpsOk = setPumpsSpeed(220.0, OFF, OFF, OFF);
        valvesOk = setValvesState(OPEN, CLOSE, CLOSE);
    }
    else if (bioreactorState == eBioreactorState::CLEANING_CIRCULATION)
    {
        pumpsOk = setPumpsSpeed(OFF, 150.0, 50.0, 50.0);
        valvesOk = setValvesState(CLOSE, OPEN, CLOSE);
    }
    else
    {
        pumpsOk = setPumpsSpeed(-220.0, -220.0, -50.0, -50.0);
        valvesOk = cleaningCycle == eCleaningCycle::DRY
                       ? setValvesState(OPEN, CLOSE, CLOSE)
                       : setValvesState(CLOSE, OPEN, OPEN);
    }

    if (!pumpsOk)
        Serial.println("ERR:CLEANING:OUTPUT_FAILURE;OUTPUT=PUMPS");
    if (!valvesOk)
        Serial.println("ERR:CLEANING:OUTPUT_FAILURE;OUTPUT=PROCESS_VALVES");
    success = valvesOk && success;
    success = pumpsOk && success;

    const bool pressureValvesOk = setPressureChamberValvesState(OFF, OFF, OFF, OFF);
    if (!pressureValvesOk)
        Serial.println("ERR:CLEANING:OUTPUT_FAILURE;OUTPUT=PRESSURE_CHAMBER_VALVES");
    success = pressureValvesOk && success;

    setPressureChamberState(OFF);
    setHeatersState(cleaningCycle == eCleaningCycle::DETERGENT);
    return success;
}

void setup()
{
    Serial.begin(SERIAL_BAUDRATE);
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Serial.println("Hello, World!");

    beginBioreactorController();
    stateTimer = millis();
    initWatchDog();
}

void loop()
{
    if (cleaningCycle == eCleaningCycle::DETERGENT &&
        isCleaningPhaseState(bioreactorState) &&
        !isCleaningTemperatureSafe())
    {
        failCleaningSequence("TEMPERATURE_SENSOR_FAILURE");
    }

    switch (bioreactorState)
    {
    case eBioreactorState::IDLE:
        setFansState(OFF, OFF, OFF, OFF, OFF, OFF, OFF);
        setPumpsSpeed(OFF, OFF, OFF, OFF);
        setValvesState(CLOSE, CLOSE, CLOSE);
        setPressureChamberValvesState(OFF, OFF, OFF, OFF);
        setHeatersState(OFF);
        stateTimer = millis();
        // switch when user command received
        break;
    case eBioreactorState::APPROV:
        // start when user send command
        setFansState(OFF, OFF, ON, ON, ON, ON, ON);
        setPumpsSpeed(120.0, OFF, OFF, OFF);
        setValvesState(OPEN, CLOSE, CLOSE); // la bonne version
        setPressureChamberState(OFF);
        setHeatersState(ON);
        // switch after 10 min to PREPARE
        if (millis() - stateTimer > 10 * MINUTE)
        {
            setBioreactorState((uint8_t)eBioreactorState::PREPARE);
            stateTimer = millis();
        }
        break;
    case eBioreactorState::PREPARE:
        // start when approv is finished
        setFansState(ON, ON, ON, ON, ON, ON, ON);
        setPumpsSpeed(OFF, 150.0, OFF, OFF); // Culture 2,  APPROV, CIRCUL, CULTURE 1
        setValvesState(CLOSE, CLOSE, CLOSE);
        setPressureChamberState(ON);
        setHeatersState(ON);
        // bool o2ValveState, co2ValveState, airValveState;
        // setPressureChamberValvesState(o2ValveState, co2ValveState, airValveState);
        //  switch when user command received (to RUN)
        break;
    case eBioreactorState::RUN:
    {
        // start when user sent command
        setFansState(ON, ON, ON, ON, ON, ON, ON);
        float circulationSpeed = 75.0f;
        float culturePump1Speed = 20.0f;
        float culturePump2Speed = 20.0f;
        getRunCulturePumpSpeeds(circulationSpeed, culturePump1Speed, culturePump2Speed);
        setPumpsSpeed(5.0f, circulationSpeed, culturePump1Speed, culturePump2Speed);
        // setPumpsSpeed(OFF, OFF, OFF, OFF); // APPROV, CIRCUL, CULTURE 2, CULTURE 1
        setValvesState(CLOSE, OPEN, CLOSE);
        setPressureChamberState(ON);
        setHeatersState(ON);
        // switch when user command received (to IDLE)
        break;
    }
    case eBioreactorState::CELL_RETURN:
        // start when user send command (from IDLE after culture is finished)
        setFansState(OFF, ON, ON, ON, ON, ON, ON);
        setPumpsSpeed(OFF, -80.0, -30.0, -30.0);
        // setPumpsSpeed(OFF, OFF, 200.0, 200.0);
        setValvesState(CLOSE, OPEN, OPEN); //       setValvesState(OPEN, OPEN, CLOSE);
        setPressureChamberState(OFF);
        setHeatersState(ON);

        // switch after 10 min to IDLE
        if (millis() - stateTimer > 8 * MINUTE)
        {
            setBioreactorState((uint8_t)eBioreactorState::TRASH);
            stateTimer = millis();
        }
        break;
    case eBioreactorState::CLEANING_APPROV:
        if (!applyCleaningPhaseOutputs())
        {
            failCleaningSequence("PHASE_OUTPUT_FAILURE");
            break;
        }

        if (millis() - stateTimer > 1 * MINUTE) // 5
            advanceCleaningPhase(eBioreactorState::CLEANING_CIRCULATION);
        break;
    case eBioreactorState::CLEANING_CIRCULATION:
        if (!applyCleaningPhaseOutputs())
        {
            failCleaningSequence("PHASE_OUTPUT_FAILURE");
            break;
        }

        if (millis() - stateTimer > 1 * MINUTE) // 15
            advanceCleaningPhase(eBioreactorState::CLEANING_RETURN);
        break;
    case eBioreactorState::CLEANING_RETURN:
        if (!applyCleaningPhaseOutputs())
        {
            failCleaningSequence("PHASE_OUTPUT_FAILURE");
            break;
        }

        if (millis() - stateTimer > 1 * MINUTE) // 5
            finishCleaningCycle();
        break;
    case eBioreactorState::CLEANING_WAITING_WATER:
        if (!setCleaningSafeOutputs())
            failCleaningSequence("WAITING_STATE_OUTPUT_FAILURE");
        break;
    case eBioreactorState::CLEANING_WAITING_DETERGENT:
        if (!setCleaningSafeOutputs())
            failCleaningSequence("WAITING_STATE_OUTPUT_FAILURE");
        break;
        /* États non utilisés
            case eBioreactorState::RINSING_APPROV:
                setFansState(OFF, OFF, ON, ON, ON, ON, ON);
                setPumpsSpeed(220.0, OFF, OFF, OFF);
                setValvesState(OPEN, CLOSE, CLOSE);
                setPressureChamberState(OFF);
                setHeatersState(OFF);

                // switch after 5 min to RINSING_CIRCULATION
                if (millis() - stateTimer > 5 * MINUTE)
                {
                    setBioreactorState((uint8_t)eBioreactorState::RINSING_CIRCULATION);
                    stateTimer = millis();
                }
                break;
            case eBioreactorState::RINSING_CIRCULATION:
                // start after rinsing liquid is approvisioned
                setFansState(OFF, OFF, ON, ON, ON, ON, ON);
                setPumpsSpeed(OFF, 150.0, 50.0, 50.0);
                setValvesState(CLOSE, OPEN, CLOSE);
                setPressureChamberState(OFF);
                setHeatersState(OFF);

                // switch after 10 min to RINSING_RETURN
                if (millis() - stateTimer > 15 * MINUTE)
                {
                    setBioreactorState((uint8_t)eBioreactorState::RINSING_RETURN);
                    stateTimer = millis();
                }
                break;
            case eBioreactorState::RINSING_RETURN:
                // start after rinsing liquid is finished
                setFansState(OFF, OFF, ON, ON, ON, ON, ON);
                setPumpsSpeed(-150.0, -100.0, -20.0, -20.0);
                setValvesState(CLOSE, OPEN, OPEN);
                setPressureChamberState(OFF);
                setHeatersState(OFF);

                // switch after 2 min to OPEN_VALVES
                if (millis() - stateTimer > 2 * MINUTE)
                {
                    setBioreactorState((uint8_t)eBioreactorState::OPEN_VALVES);
                    stateTimer = millis();
                }
                break;
            case eBioreactorState::RETURN_START:
                // start after rinsing liquid is finished
                setFansState(OFF, OFF, ON, ON, ON, ON, ON);
                setPumpsSpeed(-200.0, -140.0, CLOSE, CLOSE);
                setValvesState(CLOSE, CLOSE, OPEN);
                setPressureChamberState(OFF);
                setHeatersState(OFF);

                // switch after 10 min to IDLE
                if (millis() - stateTimer > 3 * MINUTE)
                {
                    setBioreactorState((uint8_t)eBioreactorState::IDLE);
                    stateTimer = millis();
                }
                break;
            case eBioreactorState::REDUCE_OVERFLOW: // reduce a bit the quantity of liquid in the sensor vial
                // start after rinsing liquid is finished
                setFansState(OFF, OFF, ON, ON, ON, ON, ON);
                setPumpsSpeed(OFF, -100.0, 0.0, 0.0);
                setValvesState(OPEN, OPEN, CLOSE);
                setPressureChamberState(OFF);
                setHeatersState(OFF);

                // switch after 10 min to IDLE
                if (millis() - stateTimer > 1 * MINUTE)
                {
                    setBioreactorState((uint8_t)eBioreactorState::IDLE);
                    stateTimer = millis();
                }
                break;
            case eBioreactorState::OPEN_VALVES: // For the fluidic and heating system test
                setFansState(OFF, OFF, OFF, OFF, OFF, OFF, OFF);
                setPumpsSpeed(OFF, OFF, OFF, OFF);
                setValvesState(OPEN, OPEN, OPEN);
                setPressureChamberState(OFF);
                setHeatersState(OFF);
                break;
            case eBioreactorState::SAMPLING:
                setFansState(OFF, OFF, OFF, OFF, OFF, OFF, OFF);
                setPumpsSpeed(-80.0, OFF, OFF, OFF);
                setValvesState(OPEN, CLOSE, CLOSE);
                setPressureChamberState(OFF);
                setHeatersState(OFF);

                if (millis() - stateTimer > 0.75 * MINUTE)
                {
                    setBioreactorState((uint8_t)eBioreactorState::RUN);
                    stateTimer = millis();
                }
                break;
            case eBioreactorState::HEATING:
                puissnace_pompes = 40.0f;
                setFansState(OFF, OFF, OFF, OFF, OFF, OFF, OFF);
                setPumpsSpeed(puissnace_pompes, puissnace_pompes, puissnace_pompes, puissnace_pompes);
                setValvesState(CLOSE, CLOSE, CLOSE);
                setPressureChamberState(OFF);
                setHeatersState(OFF);
                // switch after 1 min to IDLE
                if (millis() - stateTimer > 1 * MINUTE)
                {
                    if (puissnace_pompes < 200)
                    {
                        puissnace_pompes += 25;
                    }
                    stateTimer = millis();
                }
                break;
                */
    case eBioreactorState::TEST:
        // start when user sent command
        setFansState(ON, ON, ON, ON, ON, ON, ON);
        setPumpsSpeed(5.0, 86.0, 30.0, 30.0);
        // setPumpsSpeed(OFF, OFF, OFF, OFF); // APPROV, CIRCUL, CULTURE 2, CULTURE 1
        setValvesState(CLOSE, OPEN, CLOSE);
        setPressureChamberState(OFF);
        setHeatersState(OFF);
        // switch when user command received (to IDLE)
        break;
    case eBioreactorState::TRASH:
        setFansState(OFF, OFF, ON, ON, ON, ON, ON);
        setPumpsSpeed(-220.0, OFF, OFF, OFF);
        setValvesState(OPEN, CLOSE, CLOSE);
        setPressureChamberState(OFF);
        setHeatersState(OFF);
        break;
    case eBioreactorState::MEDIA_CHANGE_TRASH:
        setFansState(OFF, OFF, ON, ON, ON, ON, ON);
        setPumpsSpeed(-220.0, -220.0, OFF, OFF);
        setValvesState(CLOSE, CLOSE, OPEN);
        setPressureChamberState(ON);
        setHeatersState(ON);

        // À TESTER !
        // switch after 5 min to MEDIA_CHANGE_APPROV
        if (millis() - stateTimer > 5 * MINUTE)
        {
            setBioreactorState((uint8_t)eBioreactorState::MEDIA_CHANGE_APPROV);
            stateTimer = millis();
        }
        break;
    case eBioreactorState::MEDIA_CHANGE_APPROV:
        setFansState(OFF, OFF, ON, ON, ON, ON, ON);
        setPumpsSpeed(220.0, 220.0, OFF, OFF);
        setValvesState(CLOSE, CLOSE, OPEN);
        setPressureChamberState(ON);
        setHeatersState(ON);
        break;
    default:
        /* code */
        break;
    }

    updateSensors();
    printBioreactorStateToSerial();
    updateTemperatureController();
    updatePressureChamberController();
    updateLEDState();
    receiveSerialCommand();
    updateCO2Controller();
    //  updateBioreactorState(); // To be implemented when communication with the GUI will be available
    kickWatchDog();
}
