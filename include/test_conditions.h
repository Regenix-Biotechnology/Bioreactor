#ifndef TEST_CONDITIONS_H
#define TEST_CONDITIONS_H

#include <Arduino.h>

/**
 * @brief Condition test sequence (runs in the TEST state).
 *
 * 18 conditions of 2 h each (36 h): circulation pump speed + O2 / CO2 references of the pressure
 * chamber (table in test_conditions.cpp). The other outputs are the same as in RUN (approv pump,
 * culture pumps, valves, fans, pressure chamber ON, heater regulated at the setpoint). At the end
 * of the last condition the bioreactor switches to RUN and the O2 / CO2 references in use before
 * the test are restored.
 *
 * Serial commands (see serialReader.cpp):
 *   TEST_COND=START      start at condition 1
 *   TEST_COND=START,n    start at condition n (1..18), e.g. to resume after a reboot
 *   TEST_COND=NEXT       end the current condition now and go to the next one
 *   TEST_COND=STOP       stop the sequence (stays in TEST, normal TEST outputs)
 *   TEST_COND=STATUS     print the current step and remaining time
 */

struct TestCondition
{
    float circulationPumpSpeed; // ml/min
    float o2Reference;          // % (pressure chamber)
    float co2Reference;         // ppm (pressure chamber)
};

static constexpr uint8_t TEST_CONDITION_COUNT = 18;
static constexpr unsigned long TEST_CONDITION_DURATION_MS = 2UL * 60UL * 60UL * 1000UL; // 2 h

typedef enum
{
    TEST_COND_START_OK = 0,
    TEST_COND_START_WRONG_STATE,    // bioreactor must be in TEST
    TEST_COND_START_FLOW_TEST_MODE, // a FLOW_TEST was used since entering TEST
    TEST_COND_START_ALREADY_RUNNING,
    TEST_COND_START_INVALID_STEP
} eTestConditionStartResult;

eTestConditionStartResult startConditionTest(uint8_t firstStep);
bool stopConditionTest(const char *reason); // false if no sequence was running
bool advanceConditionTest();                // false if no sequence was running
bool isConditionTestActive();

/** @brief Timer of the sequence. Must be called in the TEST state of the main loop. */
void updateConditionTest();

const TestCondition &getCurrentTestCondition();
uint8_t getConditionTestStep(); // 1..9, 0 if not running

/** @brief "TEST_COND=IDLE" or "TEST_COND=RUNNING;STEP=..;...;REMAINING_S=.." (without prefix / ID). */
String getConditionTestStatusFields();

// Defined in bioreactor_controller.cpp: culture pump speeds used in RUN
// (culture setpoints if staged, otherwise the given default values are kept).
void getConfiguredCulturePumpSpeeds(float &culturePump1Speed, float &culturePump2Speed);

#endif // TEST_CONDITIONS_H