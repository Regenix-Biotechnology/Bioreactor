#include "TestSequence.h"

float stabTargetTemp = 0.0f;
unsigned long stabStartTime = 0;
unsigned long stabLastOutOfRangeTime = 0;
unsigned long stabElapsedResult = 0;
unsigned long stateStartTime = 0;

unsigned long stabilisationTime;

float temperatureAir;
float temperatureEau;
uint8_t testCompleted = 0X00;

eTestState currentState = eTestState::TEST_INIT;
eStabilisationState stabState = eStabilisationState::STAB_IDLE;
RampTestState currentRampTestState = RampTestState::TEST_RAMP_INIT;

/**
 * @brief Sate machine of the temperature test sequence which have 5 differents test
 */
void testSequenceTemperature()
{
    switch (currentState)
    {
    case TEST_INIT:
        setFansState(ON, ON, ON, ON, ON, ON, ON);
        setPumpsSpeed(0, 50, 150, 50); // APPROV, CULTURE 2, CIRCUL, CULTURE 1
        setValvesState(CLOSE, OPEN, CLOSE);
        setPressureChamberState(OFF);
        setHeatersState(ON);
        if (stabState == STAB_IDLE || stabState == STAB_RUNNING)
        {
            startStabilisation(TEST1_C_TEMP);
        }
        if (updateStabilisation(TEST1_C_TEMP + 6))
        {
            currentState = eTestState::TEST_STEP1_37C_10ML;
            stabState = eStabilisationState::STAB_IDLE;
        }

        break;

    case TEST_STEP1_37C_10ML:
        // 21°C(TEST1_C_TEMP) -> 37°C avec débit de 10ml/min (stabilisation 5 minutes)
        if (stabState == STAB_IDLE || stabState == STAB_RUNNING)
        {
            temperatureEau = tempSensor.getTemperatureC();
            setFansState(ON, ON, ON, ON, ON, ON, ON);
            setPumpsSpeed(OFF, OFF, (DEBIT_10ML + 5), DEBIT_10ML);
            startStabilisation(TEST1_TEMP);
        }
        if (updateStabilisation(TEST1_TEMP))
        {
            testCompleted |= 0x01;
            stabState = eStabilisationState::STAB_IDLE;
            currentState = eTestState::COOL_DOWN;
        }
        break;

    case COOL_DOWN:
        // temperatureController.setReferenceTemperature(27.0);
        setHeatersState(OFF);
        if (stabState == STAB_IDLE || stabState == STAB_RUNNING)
        {
            setPumpsSpeed(0, 50, 150, 50); // APPROV, CULTURE 2, CIRCUL, CULTURE 1
            startStabilisation(TEMP_TH_LOW);
        }
        if (updateStabilisation(TEMP_TH_LOW + 6) && testCompleted == 0X01)
        {
            currentState = eTestState::TEST_STEP2_37C_100ML;
            stabState = eStabilisationState::STAB_IDLE;
        }
        if (updateStabilisation(TEMP_TH_LOW + 6) && testCompleted == 0X03)
        {
            currentState = eTestState::TEST_STEP3_RAMP;
            stabState = eStabilisationState::STAB_IDLE;
        }
        break;

    case TEST_STEP2_37C_100ML:
        // 21°C -> 37°C avec débit de 100ml/min (stabilisation 5 minutes)
        if (stabState == STAB_IDLE || stabState == STAB_RUNNING)
        {
            temperatureEau = tempSensor.getTemperatureC();
            setFansState(ON, ON, ON, ON, ON, ON, ON);
            setPumpsSpeed(OFF, OFF, (DEBIT_100ML + 20), DEBIT_100ML);
            startStabilisation(TEST2_TEMP);
        }
        if (updateStabilisation(TEST2_TEMP))
        {
            testCompleted |= 0x02;
            stabState = eStabilisationState::STAB_IDLE;
            currentState = eTestState::COOL_DOWN;
        }
        break;
    case TEST_STEP3_RAMP:
        setFansState(ON, ON, ON, ON, ON, ON, ON);
        // setPumpsSpeed(0, 50, 150, 50); // APPROV, CULTURE 2, CIRCUL, CULTURE 1
        setValvesState(CLOSE, OPEN, CLOSE);
        setPressureChamberState(OFF);
        setHeatersState(ON);
        testSequenceTemperatureRamp();
        if (currentRampTestState == RampTestState::TEST_RAMP_DONE)
        {
            currentState = eTestState::TEST_SET_37; // pour le test on s'arrête la
            stabState = eStabilisationState::STAB_IDLE;
        }
        break;
    case TEST_SET_37:
        if (stabState == STAB_IDLE || stabState == STAB_RUNNING)
        {
            setPumpsSpeed(OFF, OFF, (DEBIT_100ML + 10), DEBIT_100ML);
            setHeatersState(ON);
            startStabilisation(37);
        }
        if (updateStabilisation(37))
        {
            stabState = eStabilisationState::STAB_IDLE;
            currentState = eTestState::TEST_STEP4_37_5;
            if (testCompleted == 0x07)
            {
                currentState = eTestState::TEST_STEP5_36_5;
            }
            else
            {
                currentState = eTestState::TEST_STEP4_37_5;
            }
        }
        break;
    case TEST_STEP4_37_5:
        if (stabState == STAB_IDLE || stabState == STAB_RUNNING)
        {
            setFansState(ON, ON, ON, ON, ON, ON, ON);
            setHeatersState(ON);
            setPumpsSpeed(OFF, OFF, (DEBIT_10ML + 10), DEBIT_10ML);
            startStabilisation(TEST4_TEMP);
        }
        else if (updateStabilisation(TEST4_TEMP))
        {
            stabState = eStabilisationState::STAB_IDLE;
            testCompleted = 0x07;
        }
        break;

    case TEST_STEP5_36_5:
        if (stabState == STAB_IDLE || stabState == STAB_RUNNING)
        {
            setFansState(ON, ON, ON, ON, ON, ON, ON);
            setPumpsSpeed(OFF, OFF, (DEBIT_10ML + 10), DEBIT_10ML);
            startStabilisation(TEST5_TEMP);
        }
        else if (updateStabilisation(TEST5_TEMP))
        {
            stabState = eStabilisationState::STAB_IDLE;
            currentState = eTestState::TEST_DONE;
        }
        break;

    case TEST_DONE:
        setHeatersState(OFF);
        break;
    }
}
/**
 * @brief State machine for the ramp test sequence (32 -> 34 -> 37 -> 40 ->
 *  42 -> 40 -> 37 -> 34 -> 32 ( 5 minutes stabilisation each)).
 */
void testSequenceTemperatureRamp()
{
    switch (currentRampTestState)
    {
    case RampTestState::TEST_RAMP_INIT:
        if (stabState == STAB_IDLE || stabState == STAB_RUNNING)
        {
            setFansState(ON, ON, ON, ON, ON, ON, ON);
            setPumpsSpeed(OFF, OFF, (DEBIT_10ML + 10), DEBIT_10ML);
            startStabilisation(32);
        }
        if (updateStabilisation(32))
        {
            stabState = eStabilisationState::STAB_IDLE;
            currentRampTestState = RampTestState::TEST_RAMP_32_34;
        }
        break;

    case RampTestState::TEST_RAMP_32_34:
        if (stabState == STAB_IDLE || stabState == STAB_RUNNING)
        {
            setFansState(ON, ON, ON, ON, ON, ON, ON);
            setPumpsSpeed(OFF, OFF, (DEBIT_10ML + 10), DEBIT_10ML);
            startStabilisation(34);
        }
        if (updateStabilisation(34))
        {
            stabState = eStabilisationState::STAB_IDLE;
            currentRampTestState = RampTestState::TEST_RAMP_34_37;
        }
        break;

    case RampTestState::TEST_RAMP_34_37:
        if (stabState == STAB_IDLE || stabState == STAB_RUNNING)
        {
            setFansState(ON, ON, ON, ON, ON, ON, ON);
            setPumpsSpeed(OFF, OFF, (DEBIT_10ML + 10), DEBIT_10ML);
            startStabilisation(37);
        }
        if (updateStabilisation(37))
        {
            stabState = eStabilisationState::STAB_IDLE;
            currentRampTestState = RampTestState::TEST_RAMP_37_40;
        }
        break;

    case RampTestState::TEST_RAMP_37_40:
        if (stabState == STAB_IDLE || stabState == STAB_RUNNING)
        {
            setFansState(ON, ON, ON, ON, ON, ON, ON);
            setPumpsSpeed(OFF, OFF, (DEBIT_10ML + 10), DEBIT_10ML);
            startStabilisation(40);
        }
        if (updateStabilisation(40))
        {
            stabState = eStabilisationState::STAB_IDLE;
            currentRampTestState = RampTestState::TEST_RAMP_40_42;
        }
        break;

    case RampTestState::TEST_RAMP_40_42:
        if (stabState == STAB_IDLE || stabState == STAB_RUNNING)
        {
            setFansState(ON, ON, ON, ON, ON, ON, ON);
            setPumpsSpeed(OFF, OFF, (DEBIT_10ML + 10), DEBIT_10ML);
            startStabilisation(42);
        }
        if (updateStabilisation(42))
        {
            stabState = eStabilisationState::STAB_IDLE;
            currentRampTestState = RampTestState::TEST_RAMP_42_40;
        }
        break;

    case RampTestState::TEST_RAMP_42_40:
        if (stabState == STAB_IDLE || stabState == STAB_RUNNING)
        {
            setFansState(ON, ON, ON, ON, ON, ON, ON);
            setPumpsSpeed(OFF, OFF, (DEBIT_10ML + 10), DEBIT_10ML);
            startStabilisation(40);
        }
        if (updateStabilisation(40))
        {
            stabState = eStabilisationState::STAB_IDLE;
            currentRampTestState = RampTestState::TEST_RAMP_40_37;
        }
        break;

    case RampTestState::TEST_RAMP_40_37:
        if (stabState == STAB_IDLE || stabState == STAB_RUNNING)
        {
            setFansState(ON, ON, ON, ON, ON, ON, ON);
            setPumpsSpeed(OFF, OFF, (DEBIT_10ML + 10), DEBIT_10ML);
            startStabilisation(37);
        }
        if (updateStabilisation(37))
        {
            stabState = eStabilisationState::STAB_IDLE;
            currentRampTestState = RampTestState::TEST_RAMP_37_34;
        }
        break;

    case RampTestState::TEST_RAMP_37_34:
        if (stabState == STAB_IDLE || stabState == STAB_RUNNING)
        {
            setFansState(ON, ON, ON, ON, ON, ON, ON);
            setPumpsSpeed(OFF, OFF, (DEBIT_10ML + 10), DEBIT_10ML);
            startStabilisation(34);
        }
        if (updateStabilisation(34))
        {
            stabState = eStabilisationState::STAB_IDLE;
            currentRampTestState = RampTestState::TEST_RAMP_34_32;
        }
        break;

    case RampTestState::TEST_RAMP_34_32:
        if (stabState == STAB_IDLE || stabState == STAB_RUNNING)
        {
            setFansState(ON, ON, ON, ON, ON, ON, ON);
            setPumpsSpeed(OFF, OFF, (DEBIT_10ML + 10), DEBIT_10ML);
            startStabilisation(32);
        }
        if (updateStabilisation(32))
        {
            stabState = eStabilisationState::STAB_IDLE;
            currentRampTestState = RampTestState::TEST_RAMP_DONE;
        }
        break;
    }
}

/**
 * @brief Get the state of the state machine "TestState" and return it.
 * @return The Current state of the testSequenceTemperature state machine.
 */
void startStabilisation(float temperature)
{
    if (stabState == eStabilisationState::STAB_IDLE)
    {
        stabStartTime = millis();
        stabLastOutOfRangeTime = millis();
        stabState = eStabilisationState::STAB_RUNNING;
    }
    temperatureController.setReferenceTemperature(temperature);
}

/**
 * @brief Updating the state of the stabilisation of the temperature.
 * @param stabTemperature , is the target temperature to stabilized on.
 * @return This returns True if the temperature remains within the specified
 * range for more than five minutes, and False otherwise.
 */
bool updateStabilisation(float stabTemperature)
{
    if (stabState == eStabilisationState::STAB_IDLE)
    {
        return false;
    }
    if (stabState == eStabilisationState::STAB_DONE)
    {
        return true;
    }
    temperatureEau = tempSensor.getTemperatureC();
    if (temperatureEau > stabTemperature - VARIATION_MAX &&
        temperatureEau < stabTemperature + VARIATION_MAX)
    {
        if (millis() - stabLastOutOfRangeTime >= STABILIZATION_DURATION_MS)
        {
            stabState = eStabilisationState::STAB_DONE;
            return true;
        }
    }
    else
    {
        stabLastOutOfRangeTime = millis();
    }
    return false;
}

/**
 * @brief Get the state of the state machine "TestState" and return it.
 * @return The Current state of the testSequenceTemperature state machine.
 */
eTestState getStatusSTATETEST()
{
    return currentState;
}

/**
 * @brief Get the state of the state machine "StabilisationState" and return it.
 * @return The Current state of the StabilisationState state machine.
 */
eStabilisationState getStatusSTAB_STATE_TEST()
{
    return stabState;
}

/**
 * @brief Setter for the bioreactor test state.
 * @param state , state research.
 */
void setBioreactorTestState(uint8_t state_int)
{
    eTestState state = (eTestState)state_int;
    if (state >= eTestState::TestState_MAX)
    {
        return; // unknown state
    }
    currentState = state;
    stabState = eStabilisationState::STAB_IDLE;
}

/**
 * @brief Setter for the state of completion.
 * @param completed , state of the completion.
 */
void setBioreacteurCompletedTest(uint8_t completed)
{
    testCompleted = completed;
}