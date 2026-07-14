#ifndef TESTSEQUENCE_H
#define TESTSEQUENCE_H
#include "main.h"
#include "bioreactor_controller.h"

#define TEST_TEMP1 31
#define VARIATION_MAX 0.2
#define STABILISATION_TIME 5000
#define STABILIZATION_DURATION_MS 300000UL // 5 minutes
#define DEBIT_10ML 10
#define DEBIT_100ML 100
#define TEMP_TH_LOW 30

// États globaux de la séquence de test
enum eTestState
{
    TEST_INIT = 0,
    TEST_STEP1_37C_10ML,
    COOL_DOWN,
    TEST_STEP2_37C_100ML,
    TEST_STEP3_RAMP,
    TEST_SET_37,
    TEST_STEP4_37_5,
    TEST_STEP5_36_5,
    TEST_DONE,
    TestState_MAX
};

// État de la sous-machine "stabilisation"
enum eStabilisationState
{
    STAB_IDLE = 0,
    STAB_RUNNING,
    STAB_DONE,
    StabilisationState_MAX
};

// État de la sous-machine "Ramp test"
enum RampTestState
{
    TESR_RAMP_INIT = 0,
    TESR_RAMP_32_34,
    TESR_RAMP_34_37,
    TESR_RAMP_37_40,
    TESR_RAMP_40_42,
    TESR_RAMP_42_40,
    TESR_RAMP_40_37,
    TESR_RAMP_37_34,
    TESR_RAMP_34_32,
    TESR_RAMP_DONE,
    RampTestState_MAX
};

extern eTestState currentState;
extern eStabilisationState stabState;
extern RampTestState currentRampTestState;

// protoypes

void startStabilisation(float temperature);
void testSequenceTemperature();
bool updateStabilisation(unsigned long &stabElapsedResult, float stabTemperature);
eTestState getStatusSTATETEST();
eStabilisationState getStatusSTAB_STATE_TEST();
void testSequenceTemperatureRamp();
void setBioreactorTestState(uint8_t state_int);

#endif