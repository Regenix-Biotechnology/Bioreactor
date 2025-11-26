#include "main.h"

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
    switch (bioreactorState)
    {
    case eBioreactorState::IDLE:
        setFansState(OFF, OFF, OFF, OFF, OFF, OFF, OFF);
        setPumpsSpeed(OFF, OFF, OFF, OFF);
        setValvesState(CLOSE, CLOSE, CLOSE);
        setPressureChamberValvesState(OFF, OFF, OFF);
        setHeatersState(OFF);
        stateTimer = millis();
        // switch when user command received
        break;
    case eBioreactorState::APPROV:
        // start when user send command
        setFansState(OFF, OFF, ON, ON, ON, ON, ON);
        setPumpsSpeed(220.0, OFF, OFF, OFF);
        setValvesState(OPEN, CLOSE, CLOSE);
        setPressureChamberState(OFF);
        setHeatersState(OFF);

        // switch after 5 min to PREPARE
        if (millis() - stateTimer > 5 * MINUTE)
        {
            setBioreactorState((uint8_t)eBioreactorState::PREPARE);
            stateTimer = millis();
        }
        break;
    case eBioreactorState::PREPARE:
        // start when approv is finished
        setFansState(ON, ON, ON, ON, ON, ON, ON);
        setPumpsSpeed(OFF, 150.0, OFF, OFF);
        setValvesState(CLOSE, CLOSE, CLOSE);
        setPressureChamberState(ON);
        setHeatersState(ON);
        // switch when user command received (to RUN)
        break;
    case eBioreactorState::RUN:
        // start when user sent command
        setFansState(ON, ON, ON, ON, ON, ON, ON);
        setPumpsSpeed(OFF, 110.0, 50.0, 50.0);
        setValvesState(CLOSE, OPEN, CLOSE);
        setPressureChamberState(ON);
        setHeatersState(ON);
        // switch when user command received (to IDLE)
        break;
    case eBioreactorState::CELL_RETURN:
        // start when user send command (from IDLE after culture is finished)
        setFansState(OFF, ON, ON, ON, ON, ON, ON);
        setPumpsSpeed(-200.0, -200.0, -50.0, -50.0);
        setValvesState(OPEN, OPEN, CLOSE);
        setPressureChamberState(OFF);
        setHeatersState(OFF);

        // switch after 10 min to IDLE
        if (millis() - stateTimer > 5 * MINUTE)
        {
            setBioreactorState((uint8_t)eBioreactorState::IDLE);
            stateTimer = millis();
        }
        break;
    case eBioreactorState::CLEANING_APPROV:
        // start when user send command (from IDLE after growth liquid returned)
        setFansState(OFF, OFF, ON, ON, ON, ON, ON);
        setPumpsSpeed(220.0, OFF, OFF, OFF);
        setValvesState(OPEN, CLOSE, CLOSE);
        setPressureChamberState(OFF);
        setHeatersState(OFF);

        // switch after 5 min to CLEANING_CIRCULATION
        if (millis() - stateTimer > 5 * MINUTE)
        {
            setBioreactorState((uint8_t)eBioreactorState::CLEANING_CIRCULATION);
            stateTimer = millis();
        }
        break;
    case eBioreactorState::CLEANING_CIRCULATION:
        // start after cleaning liquid is approvisioned
        setFansState(OFF, OFF, ON, ON, ON, ON, ON);
        setPumpsSpeed(OFF, 150.0, 50.0, 50.0);
        setValvesState(CLOSE, OPEN, CLOSE);
        setPressureChamberState(OFF);
        setHeatersState(OFF);

        // switch after 10 min to CLEANING_RETURN
        if (millis() - stateTimer > 15 * MINUTE)
        {
            setBioreactorState((uint8_t)eBioreactorState::CLEANING_RETURN);
            stateTimer = millis();
        }
        break;
    case eBioreactorState::CLEANING_RETURN:
        setFansState(OFF, OFF, ON, ON, ON, ON, ON);
        setPumpsSpeed(-220.0, -220.0, -50.0, -50.0);
        setValvesState(CLOSE, OPEN, OPEN);
        setPressureChamberState(OFF);
        setHeatersState(OFF);

        // switch after 10 min to RINSING_LIQUID_APPROV
        if (millis() - stateTimer > 5 * MINUTE)
        {
            setBioreactorState((uint8_t)eBioreactorState::IDLE);
            stateTimer = millis();
        }
        break;
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
        setPumpsSpeed(-200.0, -125.0, -50.0, -50.0);
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
    case eBioreactorState::TEST: // For the fluidic and heating system test
        setFansState(ON, ON, ON, ON, ON, ON, ON);
        setPumpsSpeed(OFF, OFF, OFF, OFF);
        setValvesState(OPEN, OPEN, OPEN);
        setPressureChamberState(ON);
        setHeatersState(OFF);
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
        setPumpsSpeed(-50.0, OFF, OFF, OFF);
        setValvesState(OPEN, CLOSE, CLOSE);
        setPressureChamberState(OFF);
        setHeatersState(OFF);
        break;
    case eBioreactorState::HEATING:
        setFansState(OFF, OFF, OFF, OFF, OFF, OFF, OFF);
        setPumpsSpeed(OFF, OFF, OFF, OFF);
        setValvesState(CLOSE, CLOSE, CLOSE);
        setPressureChamberState(OFF);
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
    // updateBioreactorState(); // To be implemented when communication with the GUI will be available
    serialReader(); // This is used for DEBUG only
    kickWatchDog();
}
