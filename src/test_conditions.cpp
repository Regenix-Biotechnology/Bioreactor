#include "test_conditions.h"
#include "main.h"
#include <math.h>

// circulation (ml/min), O2 (%), CO2 (ppm)
// pH test: O2 constant at 80 %, pump speeds 100 then 50, CO2 150 000 / 180 000 ppm, 5 h 30 per condition (22 h)
static const TestCondition TEST_CONDITIONS[TEST_CONDITION_COUNT] = {
    {100.0f, 80.0f, 150000.0f}, // 1
    {100.0f, 80.0f, 180000.0f}, // 2
    {50.0f, 80.0f, 150000.0f},  // 3
    {50.0f, 80.0f, 180000.0f},  // 4
};

static bool conditionTestActive = false;
static uint8_t conditionIndex = 0; // 0..8
static unsigned long conditionStartMs = 0;
static unsigned long sequenceStartMs = 0;
static float savedO2Reference = 0.0f;
static float savedCO2Reference = 0.0f;

// ---------------------------------------------------------------------------- persistence
// The progress is stored in flash (Preferences, namespace of bioreactorParameter) so that the
// sequence resumes after a reboot of the ESP32. Written at start, at each step change and every
// PROGRESS_SAVE_INTERVAL_MS (~270 writes for a 22 h test).
static constexpr unsigned long PROGRESS_SAVE_INTERVAL_MS = 5UL * 60UL * 1000UL; // 5 min
static unsigned long lastProgressSaveMs = 0;

static void saveProgress(unsigned long now)
{
    bioreactorParameter.putBool("tcActive", true);
    bioreactorParameter.putUChar("tcStep", conditionIndex);
    bioreactorParameter.putULong("tcStepMs", now - conditionStartMs);
    bioreactorParameter.putULong("tcSeqMs", now - sequenceStartMs);
    bioreactorParameter.putFloat("tcO2Sv", savedO2Reference);
    bioreactorParameter.putFloat("tcCO2Sv", savedCO2Reference);
    lastProgressSaveMs = now;
}

static void clearProgress()
{
    bioreactorParameter.putBool("tcActive", false);
}

// ---------------------------------------------------------------------------- helpers

static String conditionFields(uint8_t index)
{
    const TestCondition &c = TEST_CONDITIONS[index];
    return ";STEP=" + String(index + 1) +
           ";TOTAL=" + String(TEST_CONDITION_COUNT) +
           ";CIRCULATION=" + String(c.circulationPumpSpeed, 1) +
           ";O2=" + String(c.o2Reference, 1) +
           ";CO2=" + String(c.co2Reference, 0);
}

static String conditionText(uint8_t index)
{
    const TestCondition &c = TEST_CONDITIONS[index];
    return "condition " + String(index + 1) + "/" + String(TEST_CONDITION_COUNT) +
           " (circulation " + String(c.circulationPumpSpeed, 0) +
           ", O2 " + String(c.o2Reference, 0) + " %" +
           ", CO2 " + String(c.co2Reference, 0) + " ppm)";
}

static void printStepEnd(uint8_t index, unsigned long now)
{
    Serial.println("EVENT:TEST_COND:STEP_END" + conditionFields(index) +
                   ";ELAPSED_S=" + String((now - conditionStartMs) / 1000UL));
}

static void startStep(uint8_t index, unsigned long now)
{
    conditionIndex = index;
    conditionStartMs = now;
    const TestCondition &c = TEST_CONDITIONS[index];
    pressureChamber.setReferenceLevel(O2, c.o2Reference);
    pressureChamber.setReferenceLevel(CO2, c.co2Reference);
    Serial.println("EVENT:TEST_COND:STEP_START" + conditionFields(index) +
                   ";DURATION_S=" + String(TEST_CONDITION_DURATION_MS / 1000UL));
    saveProgress(now);
}

static void restoreReferences()
{
    pressureChamber.setReferenceLevel(O2, savedO2Reference);
    pressureChamber.setReferenceLevel(CO2, savedCO2Reference);
}

/**
 * @brief End the current condition and start the next one, or finish the sequence (-> RUN).
 */
static void goToNextCondition(unsigned long now)
{
    const uint8_t endedIndex = conditionIndex;
    printStepEnd(endedIndex, now);

    if (endedIndex + 1 < TEST_CONDITION_COUNT)
    {
        Serial.println("> Fin de la " + conditionText(endedIndex) +
                       " -> debut de la " + conditionText(endedIndex + 1));
        startStep(endedIndex + 1, now);
        return;
    }

    // last condition finished
    conditionTestActive = false; // before setBioreactorState(), which stops the sequence on state change
    clearProgress();
    restoreReferences();
    Serial.println("EVENT:TEST_COND:SEQUENCE_COMPLETE;TOTAL=" + String(TEST_CONDITION_COUNT) +
                   ";ELAPSED_S=" + String((now - sequenceStartMs) / 1000UL) + ";NEXT_STATE=RUN");
    Serial.println("> Fin de la " + conditionText(endedIndex) + " -> fin de la sequence, passage en RUN");
    setBioreactorState((uint8_t)eBioreactorState::RUN);
}

// ---------------------------------------------------------------------------- API

eTestConditionStartResult startConditionTest(uint8_t firstStep)
{
    if (conditionTestActive)
        return TEST_COND_START_ALREADY_RUNNING;
    if (bioreactorState != eBioreactorState::TEST)
        return TEST_COND_START_WRONG_STATE;
    if (isPressureChamberFlowTestMode())
        return TEST_COND_START_FLOW_TEST_MODE;
    if (firstStep < 1 || firstStep > TEST_CONDITION_COUNT)
        return TEST_COND_START_INVALID_STEP;

    savedO2Reference = pressureChamber.getO2Reference();
    savedCO2Reference = pressureChamber.getCO2Reference();

    const unsigned long now = millis();
    conditionTestActive = true;
    sequenceStartMs = now;
    conditionStartMs = now; // startStep() sets it again; needed for saveProgress()
    Serial.println("EVENT:TEST_COND:SEQUENCE_START;FIRST_STEP=" + String(firstStep) +
                   ";TOTAL=" + String(TEST_CONDITION_COUNT) +
                   ";STEP_DURATION_S=" + String(TEST_CONDITION_DURATION_MS / 1000UL));
    Serial.println("> Debut de la sequence de test -> debut de la " + conditionText(firstStep - 1));
    startStep(firstStep - 1, now);
    return TEST_COND_START_OK;
}

bool stopConditionTest(const char *reason)
{
    if (!conditionTestActive)
        return false;

    const unsigned long now = millis();
    conditionTestActive = false;
    clearProgress();
    restoreReferences();
    Serial.println("EVENT:TEST_COND:STOPPED" + conditionFields(conditionIndex) +
                   ";ELAPSED_S=" + String((now - conditionStartMs) / 1000UL) +
                   ";REASON=" + String(reason));
    Serial.println("> Sequence de test arretee pendant la " + conditionText(conditionIndex) +
                   " (" + String(reason) + ")");
    return true;
}

bool advanceConditionTest()
{
    if (!conditionTestActive)
        return false;
    goToNextCondition(millis());
    return true;
}

bool isConditionTestActive()
{
    return conditionTestActive;
}

void updateConditionTest()
{
    if (!conditionTestActive)
        return;

    const unsigned long now = millis();
    if (now - conditionStartMs >= TEST_CONDITION_DURATION_MS)
    {
        goToNextCondition(now);
        return;
    }
    if (now - lastProgressSaveMs >= PROGRESS_SAVE_INTERVAL_MS)
        saveProgress(now);
}

void restoreConditionTestAfterBoot()
{
    if (!bioreactorParameter.getBool("tcActive", false))
        return;

    const uint8_t step = bioreactorParameter.getUChar("tcStep", 0xFF);
    unsigned long stepMs = bioreactorParameter.getULong("tcStepMs", 0);
    const unsigned long seqMs = bioreactorParameter.getULong("tcSeqMs", 0);
    const float o2Saved = bioreactorParameter.getFloat("tcO2Sv", NAN);
    const float co2Saved = bioreactorParameter.getFloat("tcCO2Sv", NAN);

    const char *abortReason = nullptr;
    if (step >= TEST_CONDITION_COUNT || !isfinite(o2Saved) || !isfinite(co2Saved))
        abortReason = "INVALID_DATA"; // e.g. the table of conditions changed with a new firmware
    else if (bioreactorState != eBioreactorState::TEST)
        abortReason = "STATE_NOT_TEST";

    if (abortReason != nullptr)
    {
        clearProgress();
        Serial.println("EVENT:TEST_COND:RESUME_ABORTED;STEP=" + String(step + 1) +
                       ";REASON=" + String(abortReason));
        return;
    }

    if (stepMs > TEST_CONDITION_DURATION_MS)
        stepMs = TEST_CONDITION_DURATION_MS;

    const unsigned long now = millis();
    savedO2Reference = o2Saved;
    savedCO2Reference = co2Saved;
    conditionTestActive = true;
    conditionIndex = step;
    conditionStartMs = now - stepMs; // unsigned arithmetic: now - conditionStartMs == stepMs
    sequenceStartMs = now - seqMs;

    const TestCondition &c = TEST_CONDITIONS[step];
    pressureChamber.setReferenceLevel(O2, c.o2Reference);
    pressureChamber.setReferenceLevel(CO2, c.co2Reference);
    lastProgressSaveMs = now;

    Serial.println("EVENT:TEST_COND:RESUMED" + conditionFields(step) +
                   ";ELAPSED_S=" + String(stepMs / 1000UL) +
                   ";REMAINING_S=" + String((TEST_CONDITION_DURATION_MS - stepMs) / 1000UL));
    Serial.println("> Sequence de test reprise apres redemarrage : " + conditionText(step));
}

const TestCondition &getCurrentTestCondition()
{
    return TEST_CONDITIONS[conditionIndex];
}

uint8_t getConditionTestStep()
{
    return conditionTestActive ? conditionIndex + 1 : 0;
}

String getConditionTestStatusFields()
{
    if (!conditionTestActive)
        return "TEST_COND=IDLE";

    const unsigned long elapsedMs = millis() - conditionStartMs;
    const unsigned long remainingMs = elapsedMs >= TEST_CONDITION_DURATION_MS ? 0 : TEST_CONDITION_DURATION_MS - elapsedMs;
    return "TEST_COND=RUNNING" + conditionFields(conditionIndex) +
           ";ELAPSED_S=" + String(elapsedMs / 1000UL) +
           ";REMAINING_S=" + String(remainingMs / 1000UL);
}