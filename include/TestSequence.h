#ifndef TESTSEQUENCE_H
#define TESTSEQUENCE_H
#include "main.h"
#include "bioreactor_controller.h"

// target temperature for the test
#define TEST1_TEMP 37
#define TEST2_TEMP 37
#define TEST4_TEMP 37.5
#define TEST5_TEMP 36.5

// target temperature for the RAMP test
#define RAMP_TEST_T1 32
#define RAMP_TEST_T2 34
#define RAMP_TEST_T3 37
#define RAMP_TEST_T4 40
#define RAMP_TEST_T5 42

// conditional temperature for some test
#define TEST1_C_TEMP 32
#define TEMP_TH_LOW 32

#define VARIATION_MAX 0.1
#define STABILISATION_TIME 5000
#define STABILIZATION_DURATION_MS 300000UL // 5 minutes
#define DEBIT_10ML 10
#define DEBIT_100ML 100

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
    TEST_RAMP_INIT = 0,
    TEST_RAMP_32_34,
    TEST_RAMP_34_37,
    TEST_RAMP_37_40,
    TEST_RAMP_40_42,
    TEST_RAMP_42_40,
    TEST_RAMP_40_37,
    TEST_RAMP_37_34,
    TEST_RAMP_34_32,
    TEST_RAMP_DONE,
    RampTestState_MAX
};

// State machine init
// Take
// protoypes
void startStabilisation(float temperature);
void testSequenceTemperature();
bool updateStabilisation(float stabTemperature);
eTestState getStatusSTATETEST();
eStabilisationState getStatusSTAB_STATE_TEST();
void testSequenceTemperatureRamp();
void setBioreactorTestState(uint8_t state_int);
void setBioreacteurCompletedTest(uint8_t completed);

#endif