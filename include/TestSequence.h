#ifndef TESTSEQUENCE_H
#define TESTSEQUENCE_H
#include "main.h"
#include "bioreactor_controller.h"

#define TEST_TEMP1 28
#define VARIATION_MAX 0.1
#define STABILISATION_TIME 5000
#define STABILIZATION_DURATION_MS 300000UL // 5 minutes
#define DEBIT_10ML 10
#define DEBIT_100ML 100
#define TEMP_TH_LOW 26

// États globaux de la séquence de test
enum TestState
{
    TEST_INIT = 0,
    TEST_STEP1_37C_10ML,
    COOL_DOWN,
    TEST_STEP2_37C_100ML,
    TEST_STEP3_RAMP,
    TEST_STEP4_37_5,
    TEST_STEP5_36_5,
    TEST_DONE,
    TestState_MAX
};

// État de la sous-machine "stabilisation"
enum StabilisationState
{
    STAB_INIT = 0,
    STAB_IDLE,
    STAB_RUNNING,
    STAB_DONE,
    StabilisationState_MAX
};

static TestState currentState = TestState::TEST_INIT;
static StabilisationState stabState = StabilisationState::STAB_INIT;

// protoypes

void startStabilisation(float temperature);
void testSequenceTemperature();
bool updateStabilisation();
TestState getStatusSTATETEST();

#endif