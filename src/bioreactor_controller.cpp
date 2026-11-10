#include "bioreactor_controller.h"
#include <math.h>

// Objects declaration
SHT40 sht40(&Wire);
GMP251 co2Sensor(RS485_RX_PIN, RS485_TX_PIN, RS485_DE_PIN, Serial1);
O2Sensor o2Sensor;
DriveTmc5041 driveStepper1(&SPI, SPI_CS_DRV_1_PIN);
DriveTmc5041 driveStepper3(&SPI, SPI_CS_DRV_3_PIN);
StepperMotor approvPump(&driveStepper3, MOTOR_2, StepperMotor::ML_PER_REV_6_ROLLERS);
StepperMotor cultureChamberPump2(&driveStepper1, MOTOR_2, StepperMotor::ML_PER_REV_4_ROLLERS);
StepperMotor circulationPump(&driveStepper3, MOTOR_1, StepperMotor::ML_PER_REV_6_ROLLERS);
StepperMotor cultureChamberPump1(&driveStepper1, MOTOR_1, StepperMotor::ML_PER_REV_4_ROLLERS);
SSR_Relay heater(HEATER_PIN);
CO2Controller co2Controller;
IOExpander ioExpander(&Wire);
TemperatureController temperatureController;
PressureChamberController pressureChamber;
VisiFermRS485 dissolvedOxygenSensor(RS485_2_RX_PIN, RS485_2_TX_PIN, Serial2);
AtlasPHSensor pHSensor(&Wire);
AtlasTempSensor tempSensor(&Wire);
LimitSwitch limitSwitch(LIMIT_SWITCH_PIN);
LedI2C ledI2C(&Wire);
Preferences bioreactorParameter;

// Global variables
eBioreactorState bioreactorState = eBioreactorState::TEST;
eCleaningCycle cleaningCycle = eCleaningCycle::NONE;
unsigned long lastTemperatureControllerTime = 0;
unsigned long lastPressureChamberControllerTime = 0;
unsigned long lastPressureChamberControllerTimePrint = 0;
unsigned long lastPrintTime = 0;
unsigned long lastLEDUpdateTime = 0;
unsigned long lastCO2UpdateValueTime = millis() + 2200; // 2 secondes (le capteur mesure toutes les 2 secondes)
unsigned long lastCO2UpdateTime = millis() + 2200;      // 2 secondes (le capteur mesure toutes les 2 secondes)
unsigned int TSTABILISATION = 5000;
uint8_t lastLEDState = 0;
float CO2_Value = 0.0f;
unsigned long lastMotorSetSpeedTime = 0;
static float lastRequestedPumpSpeeds[4] = {NAN, NAN, NAN, NAN};
static eSHT40Status airTemperatureStatus = SHT40_STATUS_NOT_INITIALISED;
static unsigned long lastValidAirTemperatureTime = 0;
static const char *lastCleaningErrorCode = "NONE";
static float baseTemperatureReference = 37.0f;
static CultureSetpoints activeCultureSetpoints = {};
static bool cultureSetpointsAvailable = false;
uint8_t testState = 0;
unsigned long stateTimer;

namespace
{
    enum class FlowCalibrationPhase
    {
        BASELINE,
        PULSE,
        RESPONSE
    };

    constexpr unsigned long FLOW_CALIBRATION_SAMPLE_INTERVAL_MS = 1000UL;
    constexpr unsigned long FLOW_CALIBRATION_MAX_WAIT_MS = 180000UL;
    constexpr unsigned long FLOW_CALIBRATION_STABLE_DURATION_MS = 10000UL;
    constexpr unsigned long FLOW_O2_OBSERVATION_MS = 15000UL;
    constexpr size_t FLOW_CALIBRATION_WINDOW_SIZE = 15;
    constexpr size_t FLOW_CALIBRATION_REPETITIONS = 2;
    constexpr unsigned long FLOW_CALIBRATION_DURATIONS_MS[] = {175UL, 200UL, 225UL};

    bool flowCalibrationMode = false;
    bool flowCalibrationActive = false;
    bool flowCalibrationWaitingForNext = false;
    bool flowCalibrationHasRunGas = false;
    bool flowCalibrationPurgeConfirmed = false;
    bool flowCalibrationPulsePendingStart = false;
    eValves flowCalibrationValve = O2;
    eValves flowCalibrationLastValve = O2;
    size_t flowCalibrationTrialIndex = 0;
    size_t flowCalibrationRepetition = 0;
    FlowCalibrationPhase flowCalibrationPhase = FlowCalibrationPhase::BASELINE;
    unsigned long flowCalibrationPulseStartMs = 0;
    unsigned long flowCalibrationPhaseStartMs = 0;
    unsigned long flowCalibrationNextSampleMs = 0;
    unsigned long flowCalibrationStableStartMs = 0;
    float flowCalibrationBefore = 0.0f;
    float flowCalibrationPeak = 0.0f;
    unsigned long flowCalibrationPeakTimeMs = 0;
    float flowCalibrationBaselineSlope = 0.0f;
    float flowCalibrationStabilityThreshold = 0.0f;
    float flowCalibrationBaselineSamples[FLOW_CALIBRATION_WINDOW_SIZE] = {};
    float flowCalibrationResponseSamples[FLOW_CALIBRATION_WINDOW_SIZE] = {};
    size_t flowCalibrationBaselineSampleCount = 0;
    size_t flowCalibrationResponseSampleCount = 0;
    float flowCalibrationSumX2 = 0.0f;
    float flowCalibrationSumXY = 0.0f;
    float flowCalibrationNormalizedSumX2 = 0.0f;
    float flowCalibrationNormalizedSumXY = 0.0f;

    bool readFlowCalibrationValue(eValves valve, float &value)
    {
        if (valve == O2)
        {
            value = o2Sensor.getO2();
            return o2Sensor.getStatus() == O2_SENSOR_STATUS_OK && isfinite(value);
        }

        value = co2Sensor.getCO2();
        return co2Sensor.getStatus() == GMP_251_STATUS_OK && isfinite(value);
    }

    float getFlowCalibrationTrialDurationSeconds()
    {
        return static_cast<float>(FLOW_CALIBRATION_DURATIONS_MS[flowCalibrationTrialIndex]) / 1000.0f;
    }

    void calculateFlowCalibrationSlope(
        const float *samples,
        size_t count,
        float &slope,
        float &slopeStandardError)
    {
        slope = 0.0f;
        slopeStandardError = 0.0f;
        if (count < 3)
            return;

        float sumX = 0.0f;
        float sumY = 0.0f;
        float sumXX = 0.0f;
        float sumXY = 0.0f;
        for (size_t i = 0; i < count; ++i)
        {
            const float x = static_cast<float>(i);
            sumX += x;
            sumY += samples[i];
            sumXX += x * x;
            sumXY += x * samples[i];
        }

        const float countAsFloat = static_cast<float>(count);
        const float centeredXX = sumXX - sumX * sumX / countAsFloat;
        if (!(centeredXX > 0.0f))
            return;

        slope = (sumXY - sumX * sumY / countAsFloat) / centeredXX;
        float residualSumSquares = 0.0f;
        const float intercept = (sumY - slope * sumX) / countAsFloat;
        for (size_t i = 0; i < count; ++i)
        {
            const float residual = samples[i] - (intercept + slope * static_cast<float>(i));
            residualSumSquares += residual * residual;
        }

        const float residualVariance = residualSumSquares / static_cast<float>(count - 2);
        slopeStandardError = sqrtf(residualVariance / centeredXX);
    }

    void clearFlowCalibrationResponseWindow()
    {
        flowCalibrationResponseSampleCount = 0;
        flowCalibrationStableStartMs = 0;
    }

    void writeFlowCalibrationValveStatus(const char *event)
    {
        const bool outputsWritten = setPressureChamberValvesState(OFF, OFF, OFF, OFF);
        Serial.println(
            "CHAMBER_VALVES;O2=CLOSED;O2_MS=0;CO2=CLOSED;CO2_MS=0;AIR=CLOSED;AIR_MS=0;OUTPUTS=" +
            String(outputsWritten ? "OK" : "ERROR") + ";EVENT=" + String(event));
    }

    void abortPressureChamberFlowTest(const char *reason)
    {
        flowCalibrationActive = false;
        flowCalibrationWaitingForNext = false;
        flowCalibrationBaselineSampleCount = 0;
        flowCalibrationResponseSampleCount = 0;
        flowCalibrationStableStartMs = 0;
        Serial.println("ERR:FLOW_TEST:" + String(reason));
        writeFlowCalibrationValveStatus("ABORT");
    }

    void updatePressureChamberFlowTest(bool &o2ValveOpen, bool &co2ValveOpen)
    {
        if (!flowCalibrationActive || flowCalibrationWaitingForNext)
            return;

        const unsigned long now = millis();
        const unsigned long pulseDurationMs = FLOW_CALIBRATION_DURATIONS_MS[flowCalibrationTrialIndex];
        float value = 0.0f;

        if (flowCalibrationPhase == FlowCalibrationPhase::BASELINE)
        {
            if (flowCalibrationBaselineSampleCount > 0 &&
                now - flowCalibrationNextSampleMs < FLOW_CALIBRATION_SAMPLE_INTERVAL_MS)
                return;

            flowCalibrationNextSampleMs = now;
            if (!readFlowCalibrationValue(flowCalibrationValve, value))
            {
                abortPressureChamberFlowTest("SENSOR_FAILURE");
                return;
            }

            flowCalibrationBaselineSamples[flowCalibrationBaselineSampleCount++] = value;
            if (flowCalibrationBaselineSampleCount < FLOW_CALIBRATION_WINDOW_SIZE)
                return;

            flowCalibrationBefore = 0.0f;
            for (float sample : flowCalibrationBaselineSamples)
                flowCalibrationBefore += sample;
            flowCalibrationBefore /= static_cast<float>(FLOW_CALIBRATION_WINDOW_SIZE);
            if (flowCalibrationValve == O2 && flowCalibrationBefore >= 100.0f)
            {
                abortPressureChamberFlowTest("O2_HEADROOM_TOO_LOW");
                return;
            }

            float baselineSlopeStandardError = 0.0f;
            calculateFlowCalibrationSlope(
                flowCalibrationBaselineSamples,
                FLOW_CALIBRATION_WINDOW_SIZE,
                flowCalibrationBaselineSlope,
                baselineSlopeStandardError);
            flowCalibrationStabilityThreshold =
                fabsf(flowCalibrationBaselineSlope) + 3.0f * baselineSlopeStandardError;
            flowCalibrationPhase = FlowCalibrationPhase::PULSE;
            flowCalibrationPulsePendingStart = true;
            o2ValveOpen = flowCalibrationValve == O2;
            co2ValveOpen = flowCalibrationValve == CO2;
            return;
        }

        if (flowCalibrationPhase == FlowCalibrationPhase::PULSE)
        {
            if (flowCalibrationPulseStartMs == 0)
            {
                o2ValveOpen = flowCalibrationValve == O2;
                co2ValveOpen = flowCalibrationValve == CO2;
                return;
            }

            if (now - flowCalibrationPulseStartMs < pulseDurationMs)
            {
                o2ValveOpen = flowCalibrationValve == O2;
                co2ValveOpen = flowCalibrationValve == CO2;
                return;
            }

            flowCalibrationPhase = FlowCalibrationPhase::RESPONSE;
            flowCalibrationPhaseStartMs = now;
            flowCalibrationNextSampleMs = 0;
            flowCalibrationPeak = flowCalibrationBefore;
            flowCalibrationPeakTimeMs = 0;
            clearFlowCalibrationResponseWindow();
            return;
        }

        const unsigned long responseElapsedMs = now - flowCalibrationPhaseStartMs;
        if (flowCalibrationNextSampleMs == 0 ||
            now - flowCalibrationNextSampleMs >= FLOW_CALIBRATION_SAMPLE_INTERVAL_MS)
        {
            flowCalibrationNextSampleMs = now;
            if (!readFlowCalibrationValue(flowCalibrationValve, value))
            {
                abortPressureChamberFlowTest("SENSOR_FAILURE");
                return;
            }

            if (value > flowCalibrationPeak)
            {
                flowCalibrationPeak = value;
                flowCalibrationPeakTimeMs = now - flowCalibrationPulseStartMs;
            }

            if (flowCalibrationResponseSampleCount < FLOW_CALIBRATION_WINDOW_SIZE)
            {
                flowCalibrationResponseSamples[flowCalibrationResponseSampleCount++] = value;
            }
            else
            {
                for (size_t i = 1; i < FLOW_CALIBRATION_WINDOW_SIZE; ++i)
                    flowCalibrationResponseSamples[i - 1] = flowCalibrationResponseSamples[i];
                flowCalibrationResponseSamples[FLOW_CALIBRATION_WINDOW_SIZE - 1] = value;
            }

            if (flowCalibrationValve == CO2 && flowCalibrationResponseSampleCount == FLOW_CALIBRATION_WINDOW_SIZE)
            {
                float responseSlope = 0.0f;
                float responseSlopeStandardError = 0.0f;
                calculateFlowCalibrationSlope(
                    flowCalibrationResponseSamples,
                    FLOW_CALIBRATION_WINDOW_SIZE,
                    responseSlope,
                    responseSlopeStandardError);
                (void)responseSlopeStandardError;

                if (fabsf(responseSlope) <= flowCalibrationStabilityThreshold)
                {
                    if (flowCalibrationStableStartMs == 0)
                        flowCalibrationStableStartMs = now;
                }
                else
                {
                    flowCalibrationStableStartMs = 0;
                }
            }
        }

        const bool co2Stable =
            flowCalibrationValve == CO2 &&
            flowCalibrationStableStartMs != 0 &&
            now - flowCalibrationStableStartMs >= FLOW_CALIBRATION_STABLE_DURATION_MS;
        const bool o2ObservationComplete =
            flowCalibrationValve == O2 && responseElapsedMs >= FLOW_O2_OBSERVATION_MS;
        const bool timedOut = flowCalibrationValve == CO2 && responseElapsedMs >= FLOW_CALIBRATION_MAX_WAIT_MS;
        if (!co2Stable && !o2ObservationComplete && !timedOut)
            return;

        const float pulseSeconds = getFlowCalibrationTrialDurationSeconds();
        const float peakElapsedSeconds = static_cast<float>(flowCalibrationPeakTimeMs) / 1000.0f;
        const float correctedDelta = flowCalibrationPeak - flowCalibrationBefore -
                                     flowCalibrationBaselineSlope * peakElapsedSeconds;
        const float normalizedDelta = flowCalibrationValve == O2
                                          ? correctedDelta / (100.0f - flowCalibrationBefore)
                                          : 0.0f;

        flowCalibrationSumX2 += pulseSeconds * pulseSeconds;
        flowCalibrationSumXY += pulseSeconds * correctedDelta;
        if (flowCalibrationValve == O2 && flowCalibrationBefore < 100.0f)
        {
            flowCalibrationNormalizedSumX2 += pulseSeconds * pulseSeconds;
            flowCalibrationNormalizedSumXY += pulseSeconds * normalizedDelta;
        }

        const String normalizedDeltaOutput = flowCalibrationValve == O2
                                                 ? ";DELTA_NORMALIZED=" + String(normalizedDelta, 6)
                                                 : "";
        Serial.println(
            "FLOW_TEST;RESULT;GAS=" + String(flowCalibrationValve == O2 ? "O2" : "CO2") +
            ";REPETITION=" + String(flowCalibrationRepetition + 1) +
            ";PULSE_MS=" + String(pulseDurationMs) +
            ";BEFORE=" + String(flowCalibrationBefore, 2) +
            ";BASELINE_SLOPE_PER_S=" + String(flowCalibrationBaselineSlope, 4) +
            ";STABILITY_THRESHOLD_PER_S=" + String(flowCalibrationStabilityThreshold, 4) +
            ";PEAK=" + String(flowCalibrationPeak, 2) +
            ";PEAK_AFTER_S=" + String(peakElapsedSeconds, 1) +
            ";DELTA_CORRECTED=" + String(correctedDelta, 2) +
            normalizedDeltaOutput +
            ";STABILIZATION=" + String(flowCalibrationValve == O2 ? "NOT_REQUIRED" : (timedOut ? "TIMEOUT" : "OK")) +
            ";WAIT_S=" + String(static_cast<float>(responseElapsedMs) / 1000.0f, 1));

        ++flowCalibrationRepetition;
        if (flowCalibrationRepetition >= FLOW_CALIBRATION_REPETITIONS)
        {
            flowCalibrationRepetition = 0;
            ++flowCalibrationTrialIndex;
        }

        if (flowCalibrationTrialIndex >= (sizeof(FLOW_CALIBRATION_DURATIONS_MS) / sizeof(FLOW_CALIBRATION_DURATIONS_MS[0])))
        {
            const float regressionGain = flowCalibrationSumX2 > 0.0f
                                             ? flowCalibrationSumXY / flowCalibrationSumX2
                                             : 0.0f;
            if (flowCalibrationValve == O2)
            {
                const float normalizedRegressionGain = flowCalibrationNormalizedSumX2 > 0.0f
                                                           ? flowCalibrationNormalizedSumXY / flowCalibrationNormalizedSumX2
                                                           : 0.0f;
                Serial.println("FLOW_TEST;FIT;GAS=O2;GAIN_PTS_PER_S=" + String(regressionGain, 4) +
                               ";GAIN_NORMALIZED_PER_S=" + String(normalizedRegressionGain, 6));
            }
            else
            {
                Serial.println("FLOW_TEST;FIT;GAS=CO2;GAIN_PPM_PER_S=" + String(regressionGain, 2));
            }

            flowCalibrationActive = false;
            Serial.println("FLOW_TEST;STATUS=COMPLETE;GAS=" + String(flowCalibrationValve == O2 ? "O2" : "CO2"));
        }
        else
        {
            flowCalibrationWaitingForNext = true;
            Serial.println(
                "FLOW_TEST;STATUS=WAITING_FOR_NEXT;NEXT_COMMAND=FLOW_TEST=NEXT;NEXT_GAS=" +
                String(flowCalibrationValve == O2 ? "O2" : "CO2") +
                ";NEXT_REPETITION=" + String(flowCalibrationRepetition + 1) +
                ";NEXT_PULSE_MS=" + String(FLOW_CALIBRATION_DURATIONS_MS[flowCalibrationTrialIndex]));
        }

        flowCalibrationPulseStartMs = 0;
        flowCalibrationPhaseStartMs = 0;
        flowCalibrationPeakTimeMs = 0;
        clearFlowCalibrationResponseWindow();
    }

    void startNextFlowCalibrationPulse()
    {
        flowCalibrationPulseStartMs = millis();
        flowCalibrationPulsePendingStart = false;
        flowCalibrationPhaseStartMs = 0;
        flowCalibrationNextSampleMs = 0;
        flowCalibrationPeak = flowCalibrationBefore;
        flowCalibrationPeakTimeMs = 0;
        Serial.println(
            "FLOW_TEST;PULSE_START;GAS=" + String(flowCalibrationValve == O2 ? "O2" : "CO2") +
            ";DURATION_MS=" + String(FLOW_CALIBRATION_DURATIONS_MS[flowCalibrationTrialIndex]) +
            ";REPETITION=" + String(flowCalibrationRepetition + 1) +
            ";BASELINE=" + String(flowCalibrationBefore, 2) +
            ";BASELINE_SLOPE_PER_S=" + String(flowCalibrationBaselineSlope, 4) +
            ";STABILITY_THRESHOLD_PER_S=" + String(flowCalibrationStabilityThreshold, 4));
    }

    void resetFlowCalibrationFit()
    {
        flowCalibrationSumX2 = 0.0f;
        flowCalibrationSumXY = 0.0f;
        flowCalibrationNormalizedSumX2 = 0.0f;
        flowCalibrationNormalizedSumXY = 0.0f;
    }
}

static bool isCultureTemperatureState(eBioreactorState state)
{
    return state == eBioreactorState::APPROV ||
           state == eBioreactorState::PREPARE ||
           state == eBioreactorState::RUN;
}

static bool isValidCultureSetpoints(const CultureSetpoints &setpoints)
{
    return isfinite(setpoints.temperatureMin) &&
           isfinite(setpoints.temperatureMax) &&
           setpoints.temperatureMin <= setpoints.temperatureMax &&
           isfinite(setpoints.circulationPumpSpeed) &&
           setpoints.circulationPumpSpeed >= 0.0f &&
           setpoints.circulationPumpSpeed <= PUMP_MAX_SPEED &&
           isfinite(setpoints.culturePump1Speed) &&
           setpoints.culturePump1Speed >= 0.0f &&
           setpoints.culturePump1Speed <= PUMP_MAX_SPEED &&
           isfinite(setpoints.culturePump2Speed) &&
           setpoints.culturePump2Speed >= 0.0f &&
           setpoints.culturePump2Speed <= PUMP_MAX_SPEED;
}

void applyCultureTemperatureForCurrentState()
{
    float reference = baseTemperatureReference;
    if (cultureSetpointsAvailable && isCultureTemperatureState(bioreactorState))
    {
        reference = activeCultureSetpoints.temperatureMin * 0.5f +
                    activeCultureSetpoints.temperatureMax * 0.5f;
    }
    temperatureController.setReferenceTemperature(reference);
}

/**
 * @brief Call the "begin" of every objects in the bioreactor controller.
 */
void beginBioreactorController()
{
    ioExpander.begin(); // Initialize the IO Expander first to ensure a short delay before turning the valves and fans off
    sht40.begin();
    co2Sensor.begin();
    o2Sensor.begin();
    dissolvedOxygenSensor.begin();
    pHSensor.begin();
    tempSensor.begin();
    heater.begin();
    limitSwitch.begin();

    // Pumps
    SPI.begin();
    driveStepper1.begin();
    driveStepper3.begin();
    approvPump.begin();
    circulationPump.begin();
    cultureChamberPump1.begin();
    cultureChamberPump2.begin();
    beginBioreactorPreferences();
}

bool isBioreactorStateSupported(eBioreactorState state)
{
    return state == eBioreactorState::IDLE ||
           state == eBioreactorState::APPROV ||
           state == eBioreactorState::PREPARE ||
           state == eBioreactorState::RUN ||
           state == eBioreactorState::CELL_RETURN ||
           state == eBioreactorState::CLEANING_APPROV ||
           state == eBioreactorState::CLEANING_CIRCULATION ||
           state == eBioreactorState::CLEANING_RETURN ||
           state == eBioreactorState::CLEANING_WAITING_WATER ||
           state == eBioreactorState::CLEANING_WAITING_DETERGENT ||
           state == eBioreactorState::TEST ||
           state == eBioreactorState::TRASH ||
           state == eBioreactorState::MEDIA_CHANGE_TRASH ||
           state == eBioreactorState::MEDIA_CHANGE_APPROV;
}

/**
 * @brief Setter for bioreactor state
 *
 * @param state
 */
void setBioreactorState(uint8_t state_int)
{
    eBioreactorState state = (eBioreactorState)state_int;
    if (!isBioreactorStateSupported(state))
    {
        return;
    }

    const bool stateChanged = state != bioreactorState;

    const bool isCleaningState =
        state == eBioreactorState::CLEANING_APPROV ||
        state == eBioreactorState::CLEANING_CIRCULATION ||
        state == eBioreactorState::CLEANING_RETURN ||
        state == eBioreactorState::CLEANING_WAITING_WATER ||
        state == eBioreactorState::CLEANING_WAITING_DETERGENT;
    if (isCleaningState && cleaningCycle == eCleaningCycle::NONE)
    {
        return;
    }

    if (stateChanged && state != eBioreactorState::TEST)
        resetPressureChamberFlowTest();

    if (!isCleaningState)
    {
        const bool wasCleaningState =
            bioreactorState == eBioreactorState::CLEANING_APPROV ||
            bioreactorState == eBioreactorState::CLEANING_CIRCULATION ||
            bioreactorState == eBioreactorState::CLEANING_RETURN ||
            bioreactorState == eBioreactorState::CLEANING_WAITING_WATER ||
            bioreactorState == eBioreactorState::CLEANING_WAITING_DETERGENT;
        if (wasCleaningState)
        {
            setHeatersState(OFF);
        }
        cleaningCycle = eCleaningCycle::NONE;
    }

    bioreactorState = state;
    bioreactorParameter.putShort("state", (int16_t)state);
    stateTimer = millis();
    if (isCleaningState)
    {
        setHeatersState(OFF);
    }
    applyCultureTemperatureForCurrentState();
    if (stateChanged)
        Serial.println("EVENT:STATE=" + String(static_cast<int>(state)));
    return;
}

eCleaningCommandResult setCleaningCyclePhase(eCleaningCycle cycle, eBioreactorState phase)
{
    if (!isValidCleaningOperatorCommand(bioreactorState, cleaningCycle, cycle, phase))
    {
        return eCleaningCommandResult::INVALID_TRANSITION;
    }

    if (cycle == eCleaningCycle::DETERGENT &&
        phase == eBioreactorState::CLEANING_APPROV &&
        !isCleaningTemperatureSafe())
    {
        return eCleaningCommandResult::TEMPERATURE_SENSOR_NOT_READY;
    }

    if (!setCleaningSafeOutputs())
    {
        return eCleaningCommandResult::ACTUATOR_FAILURE;
    }

    lastCleaningErrorCode = "NONE";
    cleaningCycle = cycle;
    setBioreactorState((uint8_t)phase);
    setHeatersState(cycle == eCleaningCycle::DETERGENT);
    return eCleaningCommandResult::ACCEPTED;
}

const char *getLastCleaningErrorCode()
{
    return lastCleaningErrorCode;
}

void advanceCleaningPhase(eBioreactorState phase)
{
    if (!isValidCleaningPhaseTransition(bioreactorState, cleaningCycle, phase))
    {
        failCleaningSequence("INVALID_INTERNAL_TRANSITION");
        return;
    }

    setBioreactorState((uint8_t)phase);
    setHeatersState(cleaningCycle == eCleaningCycle::DETERGENT);
    Serial.println("EVENT:CLEANING:PHASE;CYCLE=" + String(cleaningCycleName(cleaningCycle)) +
                   ";PHASE=" + String(cleaningPhaseName(phase)));
    Serial.println("EVENT:CLEANING:" + String(cleaningCycleName(cleaningCycle)) + ":" +
                   String(cleaningPhaseName(phase)) + "_STARTED");
}

void finishCleaningCycle()
{
    setHeatersState(OFF);
    if (!setCleaningSafeOutputs())
    {
        failCleaningSequence("ACTUATOR_FAILURE_AT_CYCLE_END");
        return;
    }

    if (cleaningCycle == eCleaningCycle::DETERGENT)
    {
        Serial.println("EVENT:CLEANING:CYCLE_END;CYCLE=DETERGENT;PHASE=RETURN");
        setBioreactorState((uint8_t)nextCleaningStateAfterCycle(cleaningCycle));
        Serial.println("EVENT:CLEANING:WAITING_FOR_WATER;CYCLE=DETERGENT;PHASE=WAITING_FOR_WATER");
        Serial.println("EVENT:CLEANING:DETERGENT_COMPLETE");
        Serial.println("EVENT:CLEANING:WAITING_FOR_WATER_CONFIRMATION");
    }
    else if (cleaningCycle == eCleaningCycle::RINSE)
    {
        Serial.println("EVENT:CLEANING:CYCLE_END;CYCLE=RINSE;PHASE=RETURN");
        cleaningCycle = nextCleaningCycle(cleaningCycle);
        setBioreactorState((uint8_t)nextCleaningStateAfterCycle(eCleaningCycle::RINSE));
        setHeatersState(OFF);
        Serial.println("EVENT:CLEANING:CYCLE_START;CYCLE=DRY;PHASE=APPROV");
        Serial.println("EVENT:CLEANING:RINSE_COMPLETE");
        Serial.println("EVENT:CLEANING:DRY_STARTED");
        Serial.println("EVENT:CLEANING:DRY:APPROV_STARTED");
    }
    else if (cleaningCycle == eCleaningCycle::DRY)
    {
        Serial.println("EVENT:CLEANING:CYCLE_END;CYCLE=DRY;PHASE=RETURN");
        setBioreactorState((uint8_t)nextCleaningStateAfterCycle(cleaningCycle));
        Serial.println("EVENT:CLEANING:COMPLETE;CYCLE=DRY;PHASE=COMPLETE");
        Serial.println("EVENT:CLEANING:DRY_COMPLETE");
        Serial.println("EVENT:CLEANING:COMPLETE");
    }
    else
    {
        failCleaningSequence("INVALID_CYCLE_AT_CYCLE_END");
    }
}

bool setCleaningSafeOutputs()
{
    setHeatersState(OFF);
    bool success = setFansState(OFF, OFF, OFF, OFF, OFF, OFF, OFF);
    success = setValvesState(CLOSE, CLOSE, CLOSE) && success;
    success = setPressureChamberValvesState(OFF, OFF, OFF, OFF) && success;
    setPressureChamberState(OFF);
    success = setPumpsSpeed(OFF, OFF, OFF, OFF) && success;
    return success;
}

void failCleaningSequence(const char *reason)
{
    failCleaningSequence(reason, cleaningCycle, bioreactorState);
}

void failCleaningSequence(const char *reason, eCleaningCycle cycle, eBioreactorState phase)
{
    lastCleaningErrorCode = reason;
    bool safeOutputsApplied = setCleaningSafeOutputs();
    setBioreactorState((uint8_t)eBioreactorState::IDLE);
    Serial.println("EVENT:CLEANING:ERROR;CYCLE=" + String(cleaningCycleName(cycle)) +
                   ";PHASE=" + String(cleaningPhaseName(phase)) + ";CODE=" + String(reason));
    Serial.println("ERR:CLEANING:" + String(reason) + ";CYCLE=" + String(cleaningCycleName(cycle)) +
                   ";PHASE=" + String(cleaningPhaseName(phase)));
    if (!safeOutputsApplied)
        Serial.println("EVENT:CLEANING:ERROR;CYCLE=" + String(cleaningCycleName(cycle)) +
                       ";PHASE=" + String(cleaningPhaseName(phase)) +
                       ";CODE=SAFE_OUTPUT_COMMAND_FAILED");
}

void reportCleaningStartFailure(
    const char *reason,
    eCleaningCycle cycle,
    eBioreactorState phase)
{
    lastCleaningErrorCode = reason;
    const bool safeOutputsApplied = setCleaningSafeOutputs();
    Serial.println("EVENT:CLEANING:ERROR;CYCLE=" + String(cleaningCycleName(cycle)) +
                   ";PHASE=" + String(cleaningPhaseName(phase)) + ";CODE=" + String(reason));
    if (!safeOutputsApplied)
    {
        lastCleaningErrorCode = "SAFE_OUTPUT_COMMAND_FAILED";
        Serial.println("EVENT:CLEANING:ERROR;CYCLE=" + String(cleaningCycleName(cycle)) +
                       ";PHASE=" + String(cleaningPhaseName(phase)) +
                       ";CODE=SAFE_OUTPUT_COMMAND_FAILED");
    }
}

bool stopCleaningSequence()
{
    const bool cleaningActive = canStopCleaningSequence(bioreactorState);
    if (!cleaningActive)
        return false;

    const eCleaningCycle stoppedCycle = cleaningCycle;
    const eBioreactorState stoppedPhase = bioreactorState;
    bool safeOutputsApplied = setCleaningSafeOutputs();
    setBioreactorState((uint8_t)eBioreactorState::IDLE);
    if (safeOutputsApplied)
    {
        Serial.println("EVENT:CLEANING:STOPPED;CYCLE=" + String(cleaningCycleName(stoppedCycle)) +
                       ";PHASE=" + String(cleaningPhaseName(stoppedPhase)));
        Serial.println("EVENT:CLEANING:STOPPED");
    }
    else
    {
        lastCleaningErrorCode = "SAFE_OUTPUT_COMMAND_FAILED";
        Serial.println("EVENT:CLEANING:ERROR;CYCLE=" + String(cleaningCycleName(stoppedCycle)) +
                       ";PHASE=" + String(cleaningPhaseName(stoppedPhase)) +
                       ";CODE=SAFE_OUTPUT_COMMAND_FAILED");
    }
    return safeOutputsApplied;
}

/**
 * @brief Get all the culture parameter from memory
 */
void beginBioreactorPreferences()
{
    bioreactorParameter.begin("bioreactor");

    baseTemperatureReference = bioreactorParameter.getFloat("temperature", 37.0f);
    float ph = bioreactorParameter.getFloat("ph", 7.0);
    float oxyDissous = bioreactorParameter.getFloat("do", 100.0);
    float co2 = bioreactorParameter.getFloat("CO2", 50000.0);
    float dioxyg = bioreactorParameter.getFloat("O2", 85.0);
    int16_t storedState = bioreactorParameter.getShort("state", (int16_t)eBioreactorState::IDLE);
    eBioreactorState state = eBioreactorState::IDLE;
    if (storedState >= 0 && isBioreactorStateSupported((eBioreactorState)storedState))
    {
        state = (eBioreactorState)storedState;
    }

    state = getSafeStartupState(state);

    temperatureController.setReferenceTemperature(baseTemperatureReference);
    pressureChamber.setReferenceLevel(CO2, co2);
    pressureChamber.setReferenceLevel(O2, dioxyg);
    co2Controller.setReferenceLevel(4.5f);

    cultureSetpointsAvailable = false;
    if (bioreactorParameter.isKey("cultureCfg"))
    {
        const size_t configSize = bioreactorParameter.getBytesLength("cultureCfg");
        CultureSetpoints storedSetpoints;
        if (configSize == sizeof(storedSetpoints) &&
            bioreactorParameter.getBytes("cultureCfg", &storedSetpoints, sizeof(storedSetpoints)) == sizeof(storedSetpoints) &&
            isValidCultureSetpoints(storedSetpoints))
        {
            activeCultureSetpoints = storedSetpoints;
            cultureSetpointsAvailable = true;
        }
        else
        {
            Serial.println("ERR:CULTURE:STORED_CONFIG_INVALID");
        }
    }

    // ph
    // oxy_dissous
    bioreactorState = state;
    cleaningCycle = eCleaningCycle::NONE;
    applyCultureTemperatureForCurrentState();
    if (storedState != (int16_t)state)
    {
        bioreactorParameter.putShort("state", (int16_t)state);
    }
    // pump
}

eCultureSetpointResult stageCultureSetpoints(const CultureSetpoints &setpoints)
{
    if (bioreactorState != eBioreactorState::IDLE)
        return eCultureSetpointResult::INVALID_STATE;
    if (!isValidCultureSetpoints(setpoints))
        return eCultureSetpointResult::INVALID_VALUES;

    if (bioreactorParameter.putBytes("cultureCfg", &setpoints, sizeof(setpoints)) != sizeof(setpoints))
        return eCultureSetpointResult::STORAGE_FAILURE;

    activeCultureSetpoints = setpoints;
    cultureSetpointsAvailable = true;
    applyCultureTemperatureForCurrentState();
    return eCultureSetpointResult::ACCEPTED;
}

bool setBaseTemperatureReference(float temperature)
{
    if (!isfinite(temperature) ||
        bioreactorParameter.putFloat("temperature", temperature) != sizeof(float))
        return false;

    baseTemperatureReference = temperature;
    applyCultureTemperatureForCurrentState();
    return true;
}

bool getRunCulturePumpSpeeds(
    float &circulationSpeed,
    float &culturePump1Speed,
    float &culturePump2Speed)
{
    if (bioreactorState != eBioreactorState::RUN || !cultureSetpointsAvailable)
        return false;

    circulationSpeed = activeCultureSetpoints.circulationPumpSpeed;
    culturePump1Speed = activeCultureSetpoints.culturePump1Speed;
    culturePump2Speed = activeCultureSetpoints.culturePump2Speed;
    return true;
}

/**
 * @brief Set the state of the fans.
 * @param heaterFanState        State of the heater fan (ON/OFF)
 * @param circulationFanState   State of the interior circulation fan (ON/OFF) that circulates air inside the bioreactor
 * @param rightFanState         State of the right fan (ON/OFF) to cool the right side box containing the stepper pumps
 * @param leftFanState          State of the left fan (ON/OFF) to cool the left side box containing the stepper motors pumps
 * @param pcbFanState           State of the PCB fan (ON/OFF) to cool the PCB board
 * @param lowVoltFanState       State of the 24V fan (ON/OFF) to cool the low voltage power supply side of the electrical panel
 * @param highVoltFanState      State of the 120V fan (ON/OFF) to cool the high voltage power supply side of the electrical panel
 */
bool setFansState(bool heaterFanState, bool circulationFanState, bool rightFanState, bool leftFanState, bool pcbFanState, bool lowVoltFanState, bool highvoltFanState)
{
    bool success = ioExpander.setEfuse(EFUSE_FAN_HEATER_INDEX, heaterFanState);
    success = ioExpander.setEfuse(EFUSE_FAN_CIRCULATION_INDEX, circulationFanState) && success;
    success = ioExpander.setEfuse(EFUSE_FAN_RIGHT_INDEX, rightFanState) && success;
    success = ioExpander.setEfuse(EFUSE_FAN_LEFT_INDEX, leftFanState) && success;
    success = ioExpander.setEfuse(EFUSE_FAN_PCB_INDEX, pcbFanState) && success;
    success = ioExpander.setEfuse(EFUSE_FAN_LOW_VOLT_INDEX, lowVoltFanState) && success;
    success = ioExpander.setEfuse(EFUSE_FAN_HIGH_VOLT_INDEX, highvoltFanState) && success;
    return success;
}

/**
 * @brief Set the state of the valves.
 * @param valveApprovState        State of the supply valve. (OPEN/CLOSE)
 * @param valveCirculationState   State of the circulation valve. (OPEN/CLOSE)
 * @param valveCleaningState      State of the cleaning solution valve. (OPEN/CLOSE)
 */
bool setValvesState(bool valveSupplyState, bool valveCirculationState, bool valveCleaningState)
{
    const bool supplyOk = ioExpander.setEfuse(EFUSE_VALVE_SUPPLY_INDEX, valveSupplyState);
    if (!supplyOk)
    {
        Serial.print("ERR:IO:OUTPUT_FAILURE;OUTPUT=SUPPLY_VALVE;CHANNEL=");
        Serial.println(EFUSE_VALVE_SUPPLY_INDEX);
    }

    const bool circulationOk = ioExpander.setEfuse(EFUSE_VALVE_CIRCULATION_INDEX, valveCirculationState);
    if (!circulationOk)
    {
        Serial.print("ERR:IO:OUTPUT_FAILURE;OUTPUT=CIRCULATION_VALVE;CHANNEL=");
        Serial.println(EFUSE_VALVE_CIRCULATION_INDEX);
    }

    const bool returnOk = ioExpander.setEfuse(EFUSE_VALVE_RETURN_INDEX, valveCleaningState);
    if (!returnOk)
    {
        Serial.print("ERR:IO:OUTPUT_FAILURE;OUTPUT=RETURN_VALVE;CHANNEL=");
        Serial.println(EFUSE_VALVE_RETURN_INDEX);
    }

    return supplyOk && circulationOk && returnOk;
}

/**
 * @brief Set the state of the pressure chamber valves.
 * @param o2ValveState      State of the O2 valve. (OPEN/CLOSE)
 * @param co2ValveState     State of the CO2 valve. (OPEN/CLOSE)
 * @param airValveState     State of the air valve. (OPEN/CLOSE)
 */
bool setPressureChamberValvesState(bool o2, bool co2, bool air, bool co2Air)
{
    struct Ch
    {
        const char *name;
        int idx;
        bool state;
    };
    const Ch ch[] = {{"O2_VALVE", EFUSE_VALVE_O2_INDEX, o2},
                     {"CO2_VALVE", EFUSE_VALVE_CO2_INDEX, co2},
                     {"AIR_VALVE", EFUSE_VALVE_AIR_INDEX, air},
                     {"CO2_ATM_VALVE", EFUSE_VALVE_CO2_ATM_INDEX, co2Air}};
    bool success = true;
    for (const Ch &c : ch)
    {
        bool ok = ioExpander.setEfuse(c.idx, c.state);
        if (!ok)
            ok = ioExpander.setEfuse(c.idx, c.state); // un seul nouvel essai
        if (!ok)
        {
            Serial.print("ERR:IO:OUTPUT_FAILURE;OUTPUT=");
            Serial.print(c.name);
            Serial.print(";CHANNEL=");
            Serial.println(c.idx);
            success = false;
        }
    }
    return success;
}

/**
 * @brief Set the speed of the pumps And the number iof micro-steps for each pump.
 * Speed is in float ml/min and +/- for direction
 * @param approvPumpSpeed           Speed of the approv pump
 * @param circulationPumpSpeed           Speed of the sensor pump
 * @param cultureChamberPump1Speed  Speed of the culture chamber pump 1
 * @param cultureChamberPump2Speed  Speed of the culture chamber pump 2
 */
bool setPumpsSpeed(float approvPumpSpeed, float circulationPumpSpeed, float cultureChamberPump1Speed, float cultureChamberPump2Speed)
{
    const float requestedSpeeds[4] = {
        approvPumpSpeed,
        circulationPumpSpeed,
        cultureChamberPump1Speed,
        cultureChamberPump2Speed};
    bool speedChanged = false;
    for (uint8_t i = 0; i < 4; ++i)
    {
        if (requestedSpeeds[i] != lastRequestedPumpSpeeds[i])
        {
            speedChanged = true;
            break;
        }
    }

    if (speedChanged || millis() - lastMotorSetSpeedTime > MOTOR_SET_SPEED_MSG_INTERVAL)
    {
        eMotorStatus approvStatus = approvPump.setSpeed(approvPumpSpeed, 128);
        eMotorStatus circulationStatus = circulationPump.setSpeed(circulationPumpSpeed, 128);
        eMotorStatus culturePump1Status = cultureChamberPump1.setSpeed(cultureChamberPump1Speed, 128);
        eMotorStatus culturePump2Status = cultureChamberPump2.setSpeed(cultureChamberPump2Speed, 128);
        lastMotorSetSpeedTime = millis();
        bool success = approvStatus == MOTOR_STATUS_OK &&
                       circulationStatus == MOTOR_STATUS_OK &&
                       culturePump1Status == MOTOR_STATUS_OK &&
                       culturePump2Status == MOTOR_STATUS_OK;
        if (success)
        {
            for (uint8_t i = 0; i < 4; ++i)
                lastRequestedPumpSpeeds[i] = requestedSpeeds[i];
        }
        return success;
    }
    return true;
}

/**
 * @brief Set the state of the heaters.
 * @param heaterState       State of the heater. (0-100%)
 *
 */
void setHeatersState(bool heaterState)
{
    const bool temperatureDataSafe = isCleaningTemperatureSafe();
    const bool belowTemperatureSetpoint =
        temperatureDataSafe &&
        tempSensor.getTemperatureC() < temperatureController.getReferenceTemperature();

    if (heaterState &&
        isHeaterPermitted(bioreactorState, cleaningCycle, temperatureDataSafe) &&
        belowTemperatureSetpoint)
        heater.setLevel(temperatureController.getHeaterPower());
    else
    {
        heater.setLevel(OFF);
        temperatureController.reset();
    }
}

bool isCleaningTemperatureSafe()
{
    const unsigned long now = millis();
    return tempSensor.getStatus() == ATLAS_STATUS_OK &&
           tempSensor.getAgeMs() <= 3 * TEMPERATURE_CONTROLLER_UPDATE_INTERVAL &&
           isfinite(tempSensor.getTemperatureC()) &&
           airTemperatureStatus == SHT40_STATUS_OK &&
           lastValidAirTemperatureTime != 0 &&
           now - lastValidAirTemperatureTime <= 3 * TEMPERATURE_CONTROLLER_UPDATE_INTERVAL &&
           isfinite(sht40.getLastTemperature());
}

/**
 * @brief Determine if the pressure chamber needs to be pressurized and regulated.
 * @param state  State of the pressure chamber. (ON/OFF)
 */
void setPressureChamberState(bool state)
{
    pressureChamber.setPressureChamberState(state);
}

bool isPressureChamberFlowTestPurgeRequired(eValves valve)
{
    return flowCalibrationHasRunGas &&
           valve != flowCalibrationLastValve &&
           !flowCalibrationPurgeConfirmed;
}

bool startPressureChamberFlowTest(eValves valve)
{
    if (bioreactorState != eBioreactorState::TEST ||
        flowCalibrationActive ||
        (valve != O2 && valve != CO2) ||
        isPressureChamberFlowTestPurgeRequired(valve))
        return false;

    flowCalibrationMode = true;
    flowCalibrationActive = true;
    flowCalibrationWaitingForNext = false;
    flowCalibrationHasRunGas = true;
    flowCalibrationPurgeConfirmed = false;
    flowCalibrationLastValve = valve;
    flowCalibrationValve = valve;
    flowCalibrationTrialIndex = 0;
    flowCalibrationPulsePendingStart = false;
    flowCalibrationRepetition = 0;
    flowCalibrationPhase = FlowCalibrationPhase::BASELINE;
    flowCalibrationBaselineSampleCount = 0;
    flowCalibrationNextSampleMs = 0;
    flowCalibrationPulseStartMs = 0;
    flowCalibrationPhaseStartMs = 0;
    clearFlowCalibrationResponseWindow();
    resetFlowCalibrationFit();
    Serial.println("FLOW_TEST;STATUS=STARTED;GAS=" + String(valve == O2 ? "O2" : "CO2") +
                   ";DURATIONS_MS=175,200,225;REPETITIONS=2");
    Serial.println("FLOW_TEST;CHECK_BEFORE_START=VERIFY_SUPPLY_PRESSURE;P_APPROV_MODEL_PSI=30");
    Serial.println("FLOW_TEST;BASELINE_START;SAMPLES=15;INTERVAL_MS=1000");
    return true;
}

void stopPressureChamberFlowTest()
{
    flowCalibrationMode = true;
    flowCalibrationActive = false;
    flowCalibrationWaitingForNext = false;
    flowCalibrationPulsePendingStart = false;
    flowCalibrationBaselineSampleCount = 0;
    clearFlowCalibrationResponseWindow();
    writeFlowCalibrationValveStatus("STOP");
}

bool isPressureChamberFlowTestMode()
{
    return flowCalibrationMode;
}

bool isPressureChamberFlowTestActive()
{
    return flowCalibrationActive;
}

bool isPressureChamberFlowTestWaitingForNext()
{
    return flowCalibrationWaitingForNext;
}

bool advancePressureChamberFlowTest()
{
    if (!flowCalibrationActive || !flowCalibrationWaitingForNext)
        return false;

    flowCalibrationWaitingForNext = false;
    flowCalibrationPhase = FlowCalibrationPhase::BASELINE;
    flowCalibrationBaselineSampleCount = 0;
    flowCalibrationNextSampleMs = 0;
    clearFlowCalibrationResponseWindow();
    Serial.println("FLOW_TEST;ADVANCE_CONFIRMED;BASELINE_START;SAMPLES=15;INTERVAL_MS=1000");
    return true;
}

bool confirmPressureChamberFlowTestPurge()
{
    if (!flowCalibrationMode || flowCalibrationActive || !flowCalibrationHasRunGas)
        return false;

    flowCalibrationPurgeConfirmed = true;
    Serial.println("FLOW_TEST;PURGE_CONFIRMED;NEXT_GAS_MAY_START");
    return true;
}

void resetPressureChamberFlowTest()
{
    const bool wasInFlowTestMode = flowCalibrationMode;
    flowCalibrationMode = false;
    flowCalibrationActive = false;
    flowCalibrationWaitingForNext = false;
    flowCalibrationPulsePendingStart = false;
    flowCalibrationHasRunGas = false;
    flowCalibrationPurgeConfirmed = false;
    flowCalibrationBaselineSampleCount = 0;
    clearFlowCalibrationResponseWindow();
    if (wasInFlowTestMode && !setPressureChamberValvesState(OFF, OFF, OFF, OFF))
        Serial.println("ERR:FLOW_TEST:FAILED_TO_CLOSE_VALVES");
}

/**
 * @brief Update the temperature controller. Must be called in the main loop.
 */
void updateTemperatureController()
{
    if (millis() - lastTemperatureControllerTime > TEMPERATURE_CONTROLLER_UPDATE_INTERVAL)
    {
        lastTemperatureControllerTime = millis();
        float airTemperature = 0;
        airTemperatureStatus = sht40.getData(&airTemperature);
        if (airTemperatureStatus == SHT40_STATUS_OK)
            lastValidAirTemperatureTime = millis();
        float waterTemperature = tempSensor.getTemperatureC();

        if (airTemperatureStatus == SHT40_STATUS_OK &&
            tempSensor.getStatus() == ATLAS_STATUS_OK &&
            tempSensor.getAgeMs() <= 3 * TEMPERATURE_CONTROLLER_UPDATE_INTERVAL &&
            isfinite(airTemperature) &&
            isfinite(waterTemperature))
        {
            temperatureController.update(waterTemperature, airTemperature);
            setHeatersState(ON);
        }
        else
        {
            temperatureController.reset();
            setHeatersState(OFF);
        }
    }

    heater.update();
}

/**
 * @brief Update the pressure chamber controller. Must be called in the main loop.
 */
void updatePressureChamberController()
{
    if (flowCalibrationMode)
    {
        bool o2ValveOpen = false;
        bool co2ValveOpen = false;
        updatePressureChamberFlowTest(o2ValveOpen, co2ValveOpen);
        const bool outputsWritten = setPressureChamberValvesState(o2ValveOpen, co2ValveOpen, OFF, OFF);

        if (!outputsWritten)
        {
            abortPressureChamberFlowTest("OUTPUT_FAILURE");
        }
        else if (flowCalibrationPulsePendingStart)
        {
            startNextFlowCalibrationPulse();
        }

        co2Sensor.update();
        return;
    }

    bool controllerUpdated = false;
    if (millis() - lastPressureChamberControllerTime > PRESSURE_CHAMBER_CONTROLLER_UPDATE_INTERVAL)
    {
        lastPressureChamberControllerTime = millis();
        float o2Concentration = o2Sensor.getO2();
        float co2Concentration = co2Sensor.getCO2();
        float pressure = 7.5 * 6895; // 7.5 psi to Pa // TODO: get this value from the sensor

        if (o2Sensor.getStatus() == O2_SENSOR_STATUS_OK)
            pressureChamber.update(o2Concentration, co2Concentration, pressure);
        else
            pressureChamber.update(NAN, co2Concentration, pressure);

        controllerUpdated = true;
    }

    const bool o2ValveOpen = pressureChamber.getValveState(O2);
    const bool co2ValveOpen = pressureChamber.getValveState(CO2);
    const bool airValveOpen = pressureChamber.getValveState(AIR);
    const bool outputsWritten = setPressureChamberValvesState(o2ValveOpen,
                                                              co2ValveOpen,
                                                              airValveOpen,
                                                              co2Controller.getValveState());

    if (controllerUpdated)
    {
        Serial.println(
            "CHAMBER_VALVES;O2=" + String(o2ValveOpen ? "OPEN" : "CLOSED") +
            ";O2_MS=" + String(pressureChamber.getValveOpenDurationMs(O2)) +
            ";CO2=" + String(co2ValveOpen ? "OPEN" : "CLOSED") +
            ";CO2_MS=" + String(pressureChamber.getValveOpenDurationMs(CO2)) +
            ";AIR=" + String(airValveOpen ? "OPEN" : "CLOSED") +
            ";AIR_MS=" + String(pressureChamber.getValveOpenDurationMs(AIR)) +
            ";OUTPUTS=" + String(outputsWritten ? "OK" : "ERROR"));
    }

    co2Sensor.update();
}

/**
 * @brief Print all relevant information to the Serial monitor.
 *
 * @note All prints of the code should be done in this function to keep the Serial output organized.
 */
void printBioreactorStateToSerial()
{
    if (millis() - lastPrintTime > PRINT_UPDATE_INTERVAL)
    {
        const float o2Concentration = o2Sensor.getO2();
        const float co2Concentration = co2Sensor.getCO2();
        Serial.println("> Bioreactor State: " + String(static_cast<int>(bioreactorState)));
        // Serial.println("> DO Sensor (%sat): " + String(dissolvedOxygenSensor.getOxygen()));
        // Serial.println("> pH Sensor (pH): " + String(pHSensor.getPH()));
        // Serial.println("> Water Temperature (°C): " + String(tempSensor.getTemperatureC()));
        // Serial.println("> Air Temperature (°C): " + String(sht40.getLastTemperature()));
        // Serial.println("> Air Humidity (%RH): " + String(sht40.getLastHumidity()));
        Serial.println("> Heater Power (%): " + String(temperatureController.getHeaterPower()));
        Serial.println("> CO2 Concentration (ppm): " + String(co2Concentration));
        Serial.println("> CO2 Reference (ppm): " + String(pressureChamber.getCO2Reference()));
        Serial.println("> O2 Concentration (%): " + String(o2Concentration));
        Serial.println("> O2 Reference (%): " + String(pressureChamber.getO2Reference()));
        Serial.println("> O2 status: " + String(o2Sensor.getStatus()));
        // Serial.println("> PH status: " + String(pHSensor.getStatus()));
        // Serial.println("> Temperature culture status: " + String(tempSensor.getStatus()));
        Serial.println("> CO2 status: " + String(co2Sensor.getStatus()));
        Serial.println("> DO status: " + String(dissolvedOxygenSensor.getStatus()));
        // Serial.println(">  Water Temperature setpoint (°C): " + String(temperatureController.getReferenceTemperature()));
        Serial.println("> CO2 concentration (%) Bioreactor air: " + String(CO2_Value));

        /* Add more prints here*/
        // Serial.println("> TEST status: " + String(getStatusSTATETEST()));
        // Serial.println("> TEST Stabilisation status: " + String(getStatusSTAB_STATE_TEST()));

        Serial.println("");
        lastPrintTime = millis();
    }
}

/**
 * @brief Update needed sensors. Must be called in the main loop.
 */
void updateSensors()
{
    dissolvedOxygenSensor.update();
    pHSensor.update();
    tempSensor.update();
}

/**
 * @brief Update the LED state. Must be called in the main loop.
 *
 * Currently, the LED only indicates if the door is open or closed but more states can be added later (ex: error state).
 */
void updateLEDState()
{
    bool isDoorOpen = limitSwitch.getDoorState();
    eLedState ledState = isDoorOpen ? LED_STATE_DOOR_OPEN : LED_STATE_IDLE;

    // Update the LED at each change for fast response and at every LED_UPDATE_INTERVAL to ensure periodic updates
    if (ledState != lastLEDState || (millis() - lastLEDUpdateTime) > LED_UPDATE_INTERVAL)
    {
        lastLEDState = ledState;
        ledI2C.sendState(ledState);
        lastLEDUpdateTime = millis();
    }
}

/**
 * @brief Update the CO2 controller. Must be called in the main loop.
 */
void updateCO2Controller()
{
    if (millis() - lastCO2UpdateValueTime > 2000)
    {
        CO2_Value = ledI2C.getCO2Value();
        lastCO2UpdateValueTime = millis();
        // Serial.println(CO2_Value);
    }
    co2Controller.update(CO2_Value);
}