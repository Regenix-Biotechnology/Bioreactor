#include "TestSequence.h"

float temperatureAir = 37.0;
float temperatureEau = 37.0;

uint8_t testCompleted;

static float stabTargetTemp = 0.0f;
static unsigned long stabStartTime = 0;
static unsigned long stabLastOutOfRangeTime = 0;
static unsigned long stabElapsedResult = 0;
static unsigned long stateStartTime = 0;
// static float setpointTemperature = 18;

eTestState currentState = eTestState::TEST_INIT;
eStabilisationState stabState = eStabilisationState::STAB_IDLE;
RampTestState currentRampTestState = RampTestState::TESR_RAMP_INIT;

unsigned long stabilisationTime;

/**
 * @brief Sate machine of the temperature test sequence which as 5 differents test
 */
void testSequenceTemperature()
{
    switch (currentState)
    {
    case TEST_INIT:
        // Si c'est le tout premier passage dans cet état, on initialise le chrono
        /*
        if (stateStartTime == 0)
        {
            setFansState(ON, ON, ON, ON, ON, ON, ON);
            // setPumpsSpeed(0, 50, 150, 50); // APPROV, CULTURE 2, CIRCUL, CULTURE 1
            setValvesState(CLOSE, OPEN, CLOSE);
            setPressureChamberState(OFF);
            setHeatersState(OFF);
            temperatureController.setReferenceTemperature(22.0);

            stateStartTime = millis(); // On lance le chrono au moment de l'allumage
        }

        // TEMPO NON BLOQUANTE : On attend 2 secondes avant de changer d'état
        if (millis() - stateStartTime >= 2000)
        {
            temperatureController.setReferenceTemperature(23.0);
            stateStartTime = 0; // /!\ TRÈS IMPORTANT : On reset le chrono pour le prochain état
            currentState = eTestState::TEST_STEP1_37C_10ML;
            stabState = eStabilisationState::STAB_IDLE;
        }
        */
        /*
         setFansState(ON, ON, ON, ON, ON, ON, ON);
         setPumpsSpeed(0, 50, 150, 50); // APPROV, CULTURE 2, CIRCUL, CULTURE 1
         setValvesState(CLOSE, OPEN, CLOSE);
         setPressureChamberState(OFF);
         setHeatersState(ON);
         temperatureController.setReferenceTemperature(27.0);
         */
        setFansState(ON, ON, ON, ON, ON, ON, ON);
        setPumpsSpeed(0, 50, 150, 50); // APPROV, CULTURE 2, CIRCUL, CULTURE 1
        setValvesState(CLOSE, OPEN, CLOSE);
        setPressureChamberState(OFF);
        setHeatersState(ON);
        // TEST de la fonction stabilisation
        if (stabState == STAB_IDLE || stabState == STAB_RUNNING)
        {
            startStabilisation(37);
            // temperatureController.setReferenceTemperature(30);
        }
        if (updateStabilisation(stabElapsedResult, 37))
        {
            // currentState = eTestState::TestState_MAX;
            // stabState = eStabilisationState::STAB_IDLE;
            // startStabilisation(29);
            temperatureController.setReferenceTemperature(31);
        }

        break;

    case TEST_STEP1_37C_10ML:
        // 21°C(TEMP_TH_LOW) -> 37°C avec débit de 10ml/min (stabilisation 5 minutes)
        // temperatureController.setReferenceTemperature(21.0);
        setPumpsSpeed(50, 0, 150, 50); // Culture 1, Approv , CIRCUL, CULTURE 2
        if (stabState == STAB_IDLE)
        {
            temperatureController.setReferenceTemperature(22.7);
            // temperatureController.update(temperatureEau, temperatureAir);
            temperatureEau = tempSensor.getTemperatureC();
            if (temperatureEau < 29)
            {
                setFansState(ON, ON, OFF, OFF, ON, ON, ON);
                setPumpsSpeed(OFF, OFF, (DEBIT_10ML + 5), DEBIT_10ML);
                // temperatureController.setReferenceTemperature(22.5);
                startStabilisation(29.5);
            }
        }
        else if (updateStabilisation(stabElapsedResult, 29))
        {
            testCompleted |= 0x01;
            stabState = eStabilisationState::STAB_IDLE;
            currentState = eTestState::COOL_DOWN;
        }
        // temperatureController.setReferenceTemperature(20.0);
        break;

    case COOL_DOWN:
        temperatureController.setReferenceTemperature(27.0);
        if (stabState == STAB_IDLE)
        {
            setPumpsSpeed(0, 20, 10, OFF);
            startStabilisation(TEMP_TH_LOW);
        }
        if (updateStabilisation(stabElapsedResult, TEST_TEMP1) && testCompleted == 0X01)
        {
            currentState = eTestState::TEST_STEP2_37C_100ML;
            stabState = eStabilisationState::STAB_IDLE;
        }

        break;

    case TEST_STEP2_37C_100ML:
        // 21°C -> 37°C avec débit de 100ml/min (stabilisation 5 minutes)
        if (stabState == STAB_IDLE)
        {
            temperatureEau = tempSensor.getTemperatureC();
            if (temperatureEau < TEMP_TH_LOW)
            {
                setPumpsSpeed(OFF, OFF, (DEBIT_100ML + 10), DEBIT_100ML);
                startStabilisation(37.0);
            }
        }
        else if (updateStabilisation(stabElapsedResult, TEST_TEMP1))
        {
            testCompleted |= 0x02;
            stabState = eStabilisationState::STAB_IDLE;
            currentState = eTestState::TEST_STEP3_RAMP;
        }
        break;

    case TEST_STEP3_RAMP:
        testSequenceTemperatureRamp();
        if (currentRampTestState == RampTestState::TESR_RAMP_DONE)
        {
            currentState = eTestState::TEST_SET_37;
            stabState = eStabilisationState::STAB_IDLE;
        }
        break;
    case TEST_SET_37:
        if (stabState == STAB_IDLE)
        {
            setPumpsSpeed(OFF, OFF, (DEBIT_10ML + 10), DEBIT_10ML);
            startStabilisation(37.0);
        }
        else if (updateStabilisation(stabElapsedResult, 37))
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
        if (stabState == STAB_IDLE)
        {
            temperatureEau = tempSensor.getTemperatureC();
            if (temperatureEau < 37)
            {
                setPumpsSpeed(OFF, OFF, (DEBIT_10ML + 10), DEBIT_10ML);
                startStabilisation(37.5);
            }
        }
        else if (updateStabilisation(stabElapsedResult, TEST_TEMP1))
        {
            stabState = eStabilisationState::STAB_IDLE;
            testCompleted = 0x07;
        }
        break;

    case TEST_STEP5_36_5:
        // 37 à 36.5 (stabilisation 5 minutes)
        if (stabState == STAB_IDLE)
        {
            setPumpsSpeed(OFF, OFF, (DEBIT_10ML + 10), DEBIT_10ML);
            startStabilisation(36.5);
        }
        else if (updateStabilisation(stabElapsedResult, TEST_TEMP1))
        {
            stabState = eStabilisationState::STAB_IDLE;
            currentState = eTestState::TEST_DONE;
        }
        break;

    case TEST_DONE:
        // séquence terminée — ne fait plus rien, ou déclenche un flag
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
    case RampTestState::TESR_RAMP_INIT:
        temperatureController.setReferenceTemperature(32.0);
        if (stabState == STAB_IDLE)
        {
            setPumpsSpeed(OFF, OFF, (DEBIT_10ML + 10), DEBIT_10ML);
            startStabilisation(32);
        }
        if (updateStabilisation(stabElapsedResult, 32))
        {
            stabState = eStabilisationState::STAB_IDLE;
            currentRampTestState = RampTestState::TESR_RAMP_32_34;
        }
        break;

    case RampTestState::TESR_RAMP_32_34:
        if (stabState == STAB_IDLE)
        {
            setPumpsSpeed(OFF, OFF, (DEBIT_10ML + 10), DEBIT_10ML);
            startStabilisation(34);
        }
        if (updateStabilisation(stabElapsedResult, 34))
        {
            stabState = eStabilisationState::STAB_IDLE;
            currentRampTestState = RampTestState::TESR_RAMP_34_37;
        }
        break;

    case RampTestState::TESR_RAMP_34_37:
        if (stabState == STAB_IDLE)
        {
            setPumpsSpeed(OFF, OFF, (DEBIT_10ML + 10), DEBIT_10ML);
            startStabilisation(37);
        }
        if (updateStabilisation(stabElapsedResult, 37))
        {
            stabState = eStabilisationState::STAB_IDLE;
            currentRampTestState = RampTestState::TESR_RAMP_37_40;
        }
        break;

    case RampTestState::TESR_RAMP_37_40:
        if (stabState == STAB_IDLE)
        {
            startStabilisation(40);
        }
        if (updateStabilisation(stabElapsedResult, 40))
        {
            stabState = eStabilisationState::STAB_IDLE;
            currentRampTestState = RampTestState::TESR_RAMP_40_42;
        }
        break;

    case RampTestState::TESR_RAMP_40_42:
        if (stabState == STAB_IDLE)
        {
            startStabilisation(42);
        }
        if (updateStabilisation(stabElapsedResult, 42))
        {
            stabState = eStabilisationState::STAB_IDLE;
            currentRampTestState = RampTestState::TESR_RAMP_42_40;
        }
        break;

    case RampTestState::TESR_RAMP_42_40:
        if (stabState == STAB_IDLE)
        {
            startStabilisation(40);
        }
        if (updateStabilisation(stabElapsedResult, 40))
        {
            stabState = eStabilisationState::STAB_IDLE;
            currentRampTestState = RampTestState::TESR_RAMP_40_37;
        }
        break;

    case RampTestState::TESR_RAMP_40_37:
        if (stabState == STAB_IDLE)
        {
            startStabilisation(37);
        }
        else if (updateStabilisation(stabElapsedResult, 37))
        {
            stabState = eStabilisationState::STAB_IDLE;
            currentRampTestState = RampTestState::TESR_RAMP_37_34;
        }
        break;

    case RampTestState::TESR_RAMP_37_34:
        if (stabState == STAB_IDLE)
        {
            startStabilisation(34);
        }
        else if (updateStabilisation(stabElapsedResult, 34))
        {
            stabState = eStabilisationState::STAB_IDLE;
            currentRampTestState = RampTestState::TESR_RAMP_34_32;
        }
        break;

    case RampTestState::TESR_RAMP_34_32:
        if (stabState == STAB_IDLE)
        {
            startStabilisation(32);
        }
        else if (updateStabilisation(stabElapsedResult, 32))
        {
            stabState = eStabilisationState::STAB_IDLE;
            currentRampTestState = RampTestState::TESR_RAMP_DONE;
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

// À appeler à chaque tour de loop() — non-bloquant.
// Retourne true quand la stabilisation est terminée.
bool updateStabilisation(unsigned long &stabElapsedResult, float stabTemperature)
{
    if (stabState == eStabilisationState::STAB_IDLE)
    {
        // Rien n'a été démarré
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
            stabElapsedResult = stabLastOutOfRangeTime - stabStartTime;
            stabState = eStabilisationState::STAB_DONE;
            return true;
        }
    }
    else
    {
        // On est sorti de la plage : on reset le chrono de stabilité
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
 * @brief Get the state of the state machine "TestState" and return it.
 * @return The Current state of the testSequenceTemperature state machine.
 */
eStabilisationState getStatusSTAB_STATE_TEST()
{
    return stabState;
}

/**
 * @brief Setter for the bioreactor state
 * @param state
 */
void setBioreactorTestState(uint8_t state_int)
{
    eTestState state = (eTestState)state_int;
    if (state >= eTestState::TestState_MAX)
    {
        return;
    }

    currentState = state;
    // bioreactorParameter.putShort("state", (int16_t)state);
    // stateTimer = millis();
    return;
}