#include "TestSequence.h"

float temperatureAir = 37.0;
float temperatureEau = 37.0;

uint8_t testCompleted;

static float stabTargetTemp = 0.0f;
static unsigned long stabStartTime = 0;
static unsigned long stabLastOutOfRangeTime = 0;
static unsigned long stabElapsedResult = 0;

unsigned long stabilisationTime;
// Machine à états principale de la séquence — à appeler à chaque loop()
/**
 * @brief Sate machine of the temperature test sequence which as 5 differents test
 */
void testSequenceTemperature()
{
    switch (currentState)
    {
    case TEST_INIT:
        setFansState(ON, ON, ON, ON, ON, ON, ON);
        setPumpsSpeed(OFF, OFF, OFF, OFF);
        setValvesState(CLOSE, OPEN, CLOSE);
        setPressureChamberState(OFF);
        setHeatersState(ON);
        temperatureController.setReferenceTemperature(21.0);
        currentState = TestState::TEST_STEP1_37C_10ML;
        break;

    case TEST_STEP1_37C_10ML:
        // 21°C(TEMP_TH_LOW) -> 37°C avec débit de 10ml/min (stabilisation 5 minutes)
        if (stabState == STAB_INIT)
        {
            temperatureController.update(temperatureEau, temperatureAir);
            if (temperatureEau < TEMP_TH_LOW)
            {
                setFansState(ON, ON, OFF, OFF, ON, ON, ON);
                setPumpsSpeed(OFF, (DEBIT_10ML + 5), DEBIT_10ML, OFF);
                startStabilisation(28.0);
                stabState = StabilisationState::STAB_IDLE;
            }
        }
        else if (updateStabilisation())
        {
            testCompleted |= 0x01;
            stabState = StabilisationState::STAB_INIT;
            currentState = TestState::COOL_DOWN;
        }
        break;

    case COOL_DOWN:
        if (stabState == STAB_INIT)
        {
            setPumpsSpeed(OFF, OFF, OFF, OFF);
            stabState = StabilisationState::STAB_IDLE;
        }
        startStabilisation(21.0);
        if (updateStabilisation && testCompleted == 0X01)
        {
            currentState = TestState::TEST_STEP2_37C_100ML;
        }

        break;

    case TEST_STEP2_37C_100ML:
        // 21°C -> 37°C avec débit de 100ml/min (stabilisation 5 minutes)
        if (stabState == STAB_INIT)
        {
            temperatureController.update(temperatureEau, temperatureAir);
            if (temperatureEau < TEMP_TH_LOW)
            {
                setPumpsSpeed(OFF, (DEBIT_100ML + 10), DEBIT_100ML, OFF);
                startStabilisation(37.0);
                stabState = StabilisationState::STAB_IDLE;
            }
        }
        else if (updateStabilisation())
        {
            testCompleted |= 0x02;
            stabState = StabilisationState::STAB_IDLE;
            currentState = TestState::TEST_STEP3_RAMP;
        }
        break;

    case TEST_STEP3_RAMP:
        // 32 -> 34 -> 37 -> 40 -> 42 -> 40 -> 37 -> 34 -> 32 (stabilisation 5 minutes chacun)
        // voir version détaillée plus bas avec sous-état de rampe
        currentState = TestState::TEST_STEP4_37_5;
        break;

    case TEST_STEP4_37_5:
        // 37 à 37.5 (stabilisation 5 minutes)
        if (stabState == STAB_IDLE)
        {
            startStabilisation(37.5);
        }
        else if (updateStabilisation())
        {
            stabState = StabilisationState::STAB_IDLE;
            currentState = TestState::TEST_DONE;
        }
        break;

    case TEST_STEP5_36_5:
        // 37 à 36.5 (stabilisation 5 minutes)
        if (stabState == STAB_IDLE)
        {
            startStabilisation(36.5);
        }
        else if (updateStabilisation())
        {
            stabState = StabilisationState::STAB_IDLE;
            currentState = TestState::TEST_DONE;
        }
        break;

    case TEST_DONE:
        // séquence terminée — ne fait plus rien, ou déclenche un flag
        break;
    }
}

// Lance une nouvelle stabilisation vers une température donnée
/**
 * @brief Start a stabilisation on a target temperature
 * @param temperature The target temperature in °Celcus .
 */
void startStabilisation(float temperature)
{
    stabTargetTemp = temperature;
    temperatureController.setReferenceTemperature(temperature);
    stabStartTime = millis();
    stabLastOutOfRangeTime = millis();
    stabState = StabilisationState::STAB_RUNNING;
}

// À appeler à chaque tour de loop() — non-bloquant.
// Retourne true quand la stabilisation est terminée.
/**
 * @brief Check if the stabilidation is achievded or not.
 * @return The state of the stabilisation (true = achieved,false = sitll processing).
 */
bool updateStabilisation()
{
    if (stabState != STAB_RUNNING)
    {
        return (stabState == STAB_DONE);
    }
    temperatureController.update(temperatureEau, temperatureAir);

    if (temperatureEau > stabTargetTemp - VARIATION_MAX &&
        temperatureEau < stabTargetTemp + VARIATION_MAX)
    {
        if (millis() - stabLastOutOfRangeTime >= STABILIZATION_DURATION_MS)
        {
            stabElapsedResult = millis() - stabStartTime;
            stabState = StabilisationState::STAB_DONE;
            return true;
        }
    }
    else
    {
        stabLastOutOfRangeTime = millis();
    }
    return false;
}

TestState getStatusSTATETEST()
{
    return currentState;
}