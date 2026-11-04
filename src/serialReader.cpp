#include "serialReader.h"
#include "serial_protocol.h"
#include <stdlib.h>
#include <math.h>

static String responseIdSuffix(bool hasId, uint32_t id)
{
    return hasId ? ";ID=" + String(id) : "";
}

static void printContextError(
    const char *prefix,
    const char *code,
    const String &cycle,
    const String &phase,
    bool hasId,
    uint32_t id)
{
    String response = String(prefix) + code;
    if (cycle.length() > 0)
        response += ";CYCLE=" + cycle;
    if (phase.length() > 0)
        response += ";PHASE=" + phase;
    response += responseIdSuffix(hasId, id);
    Serial.println(response);
}

static void printCleaningCommandError(
    const char *code,
    const String &cycle,
    const String &phase,
    bool hasId,
    uint32_t id)
{
    printContextError("ERR:CLEANING:", code, cycle, phase, hasId, id);
}

static void printCommandError(
    const char *code,
    bool hasId,
    uint32_t id)
{
    if (isCleaningManagedState(bioreactorState))
    {
        printContextError(
            "ERR:COMMAND:",
            code,
            cleaningCycleName(cleaningCycle),
            cleaningPhaseName(bioreactorState),
            hasId,
            id);
    }
    else
    {
        Serial.println("ERR:COMMAND:" + String(code) + responseIdSuffix(hasId, id));
    }
}

static void printStatusSnapshot(bool hasId, uint32_t id)
{
    const bool waitingForWater = isWaitingForWater(bioreactorState);
    const bool waitingForDetergent = isWaitingForDetergent(bioreactorState);
    const String suffix = responseIdSuffix(hasId, id);
    Serial.println("STATUS:STATE=" + String(static_cast<int>(bioreactorState)) +
                   ";CYCLE=" + String(cleaningCycleName(cleaningCycle)) +
                   ";PHASE=" + String(cleaningPhaseName(bioreactorState)) +
                   ";ERROR=" + String(getLastCleaningErrorCode()) + suffix);
    Serial.println(String("STATUS:WAITING_FOR_WATER=") +
                   (waitingForWater ? "yes" : "no") + suffix);
    Serial.println(String("STATUS:WAITING_FOR_DETERGENT=") +
                   (waitingForDetergent ? "yes" : "no") + suffix);
}

static bool parseStateName(const String &name, eBioreactorState &state)
{
    if (name == "IDLE")
        state = eBioreactorState::IDLE;
    else if (name == "APPROV")
        state = eBioreactorState::APPROV;
    else if (name == "PREPARE")
        state = eBioreactorState::PREPARE;
    else if (name == "RUN")
        state = eBioreactorState::RUN;
    else if (name == "CELL-RETURN")
        state = eBioreactorState::CELL_RETURN;
    else if (name == "CLEANING-APPROV")
        state = eBioreactorState::CLEANING_APPROV;
    else if (name == "CLEANING-CIRCULATION")
        state = eBioreactorState::CLEANING_CIRCULATION;
    else if (name == "CLEANING-RETURN")
        state = eBioreactorState::CLEANING_RETURN;
    else if (name == "TEST")
        state = eBioreactorState::TEST;
    else if (name == "TRASH")
        state = eBioreactorState::TRASH;
    else if (name == "MEDIA-CHANGE-TRASH" || name == "MEDIA_CHANGE_TRASH")
        state = eBioreactorState::MEDIA_CHANGE_TRASH;
    else if (name == "MEDIA-CHANGE-APPROV" || name == "MEDIA_CHANGE_APPROV")
        state = eBioreactorState::MEDIA_CHANGE_APPROV;
    else
        return false;

    return true;
}

static bool isCleaningPhase(eBioreactorState state)
{
    return isCleaningManagedState(state);
}

static bool parseCleaningCycle(const String &name, eCleaningCycle &cycle)
{
    if (name == "DETERGENT")
        cycle = eCleaningCycle::DETERGENT;
    else if (name == "RINSE")
        cycle = eCleaningCycle::RINSE;
    else if (name == "DRY")
        cycle = eCleaningCycle::DRY;
    else
        return false;

    return true;
}

static bool parseCleaningPhase(const String &name, eBioreactorState &phase)
{
    if (name == "WAITING_FOR_DETERGENT")
        phase = eBioreactorState::CLEANING_WAITING_DETERGENT;
    else if (name == "APPROV")
        phase = eBioreactorState::CLEANING_APPROV;
    else if (name == "CIRCULATION")
        phase = eBioreactorState::CLEANING_CIRCULATION;
    else if (name == "RETURN")
        phase = eBioreactorState::CLEANING_RETURN;
    else
        return false;

    return true;
}

static bool parseNamedFloatField(const String &field, const char *name, float &value)
{
    const int separator = field.indexOf(':');
    if (separator <= 0 || field.substring(0, separator) != name)
        return false;

    const String number = field.substring(separator + 1);
    const char *text = number.c_str();
    char *end = nullptr;
    value = strtof(text, &end);
    return end != text && *end == '\0' && isfinite(value);
}

static bool parseCultureSetpoints(const String &payload, CultureSetpoints &setpoints)
{
    const char *fieldNames[] = {
        "TEMP_MIN",
        "TEMP_MAX",
        "CIRCULATION_PUMP",
        "CULTURE_PUMP_1",
        "CULTURE_PUMP_2"};
    float values[5];
    int start = 0;

    for (size_t i = 0; i < 5; ++i)
    {
        const int separator = payload.indexOf(',', start);
        const bool isLastField = i == 4;
        if ((isLastField && separator >= 0) || (!isLastField && separator < 0))
            return false;

        const int end = separator < 0 ? payload.length() : separator;
        if (!parseNamedFloatField(payload.substring(start, end), fieldNames[i], values[i]))
            return false;
        start = end + 1;
    }

    setpoints.temperatureMin = values[0];
    setpoints.temperatureMax = values[1];
    setpoints.circulationPumpSpeed = values[2];
    setpoints.culturePump1Speed = values[3];
    setpoints.culturePump2Speed = values[4];
    return true;
}

void receiveSerialCommand()
{
    if (Serial.available())
    {
        String rx = Serial.readStringUntil('\n');
        rx.trim();
        size_t commandLength = 0;
        bool hasId = false;
        uint32_t commandId = 0;
        if (!extractCommandId(rx.c_str(), rx.length(), commandLength, hasId, commandId))
        {
            printCommandError("INVALID_ID", false, 0);
            return;
        }
        String command = rx.substring(0, commandLength);
        const String idSuffix = responseIdSuffix(hasId, commandId);

        if (isStatusQuery(command.c_str(), command.length()))
        {
            printStatusSnapshot(hasId, commandId);
            return;
        }

        if (command == "CLEANING=STOP")
        {
            const eCleaningCycle stoppedCycle = cleaningCycle;
            const eBioreactorState stoppedPhase = bioreactorState;
            const bool wasCleaningActive = isCleaningManagedState(bioreactorState);
            if (stopCleaningSequence())
                Serial.println("OK:CLEANING=STOP" + idSuffix);
            else if (wasCleaningActive)
                printCleaningCommandError(
                    "SAFE_OUTPUT_COMMAND_FAILED",
                    cleaningCycleName(stoppedCycle),
                    cleaningPhaseName(stoppedPhase),
                    hasId,
                    commandId);
            else
                printCleaningCommandError("NOT_ACTIVE", "", "", hasId, commandId);
            return;
        }

        if (command.startsWith("CLEANING="))
        {
            const int separator = command.indexOf(',');
            if (separator < 0 || command.indexOf(',', separator + 1) >= 0)
            {
                printCleaningCommandError(
                    "FORMAT",
                    separator >= 0 ? command.substring(9, separator) : command.substring(9),
                    separator >= 0 ? command.substring(separator + 1) : "",
                    hasId,
                    commandId);
                return;
            }

            const String cycleName = command.substring(9, separator);
            const String phaseName = command.substring(separator + 1);
            eCleaningCycle cycle;
            eBioreactorState phase;
            if (!parseCleaningCycle(cycleName, cycle))
            {
                printCleaningCommandError(
                    "UNKNOWN_CYCLE", cycleName, phaseName, hasId, commandId);
                return;
            }
            if (!parseCleaningPhase(phaseName, phase))
            {
                printCleaningCommandError(
                    "UNKNOWN_PHASE", cycleName, phaseName, hasId, commandId);
                return;
            }

            eCleaningCommandResult result = setCleaningCyclePhase(cycle, phase);
            if (result == eCleaningCommandResult::ACCEPTED)
            {
                Serial.println("OK:" + command + idSuffix);
                if (phase == eBioreactorState::CLEANING_WAITING_DETERGENT)
                {
                    Serial.println("EVENT:CLEANING:WAITING_FOR_DETERGENT;CYCLE=DETERGENT;PHASE=WAITING_FOR_DETERGENT");
                    Serial.println("EVENT:CLEANING:WAITING_FOR_DETERGENT_CONFIRMATION");
                }
                else
                {
                    Serial.println("EVENT:CLEANING:CYCLE_START;CYCLE=" +
                                   String(cleaningCycleName(cycle)) + ";PHASE=" +
                                   String(cleaningPhaseName(phase)));
                    Serial.println(cycle == eCleaningCycle::DETERGENT
                                       ? "EVENT:CLEANING:DETERGENT_STARTED"
                                       : "EVENT:CLEANING:RINSE_STARTED");
                    Serial.println("EVENT:CLEANING:" + cycleName + ":APPROV_STARTED");
                }
            }
            else if (result == eCleaningCommandResult::ACTUATOR_FAILURE)
            {
                reportCleaningStartFailure("ACTUATOR_FAILURE_AT_START", cycle, phase);
                printCleaningCommandError(
                    "ACTUATOR_FAILURE_AT_START",
                    cycleName,
                    phaseName,
                    hasId,
                    commandId);
            }
            else if (result == eCleaningCommandResult::TEMPERATURE_SENSOR_NOT_READY)
                printCleaningCommandError(
                    "TEMPERATURE_SENSOR_NOT_READY",
                    cycleName,
                    phaseName,
                    hasId,
                    commandId);
            else
                printCleaningCommandError(
                    "INVALID_TRANSITION",
                    cycleName,
                    phaseName,
                    hasId,
                    commandId);
            return;
        }

        if (command.startsWith("CULTURE="))
        {
            if (!hasId)
            {
                Serial.println("ERR:CULTURE:ID_REQUIRED");
                return;
            }

            CultureSetpoints setpoints;
            if (!parseCultureSetpoints(command.substring(8), setpoints))
            {
                Serial.println("ERR:CULTURE:FORMAT" + idSuffix);
                return;
            }

            const eCultureSetpointResult result = stageCultureSetpoints(setpoints);
            if (result == eCultureSetpointResult::ACCEPTED)
                Serial.println("OK:CULTURE;STATUS=STAGED" + idSuffix);
            else if (result == eCultureSetpointResult::INVALID_STATE)
                Serial.println("ERR:CULTURE:STATE;STATE=" +
                               String(static_cast<int>(bioreactorState)) + idSuffix);
            else if (result == eCultureSetpointResult::INVALID_VALUES)
                Serial.println("ERR:CULTURE:RANGE" + idSuffix);
            else
                Serial.println("ERR:CULTURE:STORAGE" + idSuffix);
            return;
        }

        if (command.startsWith("STATE="))
        {
            String name = command.substring(6);
            eBioreactorState state;
            if (!parseStateName(name, state))
            {
                char *end = nullptr;
                const char *value = name.c_str();
                long stateNumber = strtol(value, &end, 10);
                if (end == value || *end != '\0' ||
                    stateNumber < 0 ||
                    stateNumber > 255 ||
                    !isBioreactorStateSupported((eBioreactorState)stateNumber))
                {
                    if (isCleaningManagedState(bioreactorState))
                    {
                        printContextError(
                            "ERR:STATE:",
                            "UNKNOWN_OR_INVALID_STATE",
                            cleaningCycleName(cleaningCycle),
                            cleaningPhaseName(bioreactorState),
                            hasId,
                            commandId);
                    }
                    else
                        Serial.println("ERR:STATE:UNKNOWN_OR_INVALID" + idSuffix);
                    return;
                }
                state = (eBioreactorState)stateNumber;
            }

            const bool cleaningActive =
                bioreactorState == eBioreactorState::CLEANING_APPROV ||
                bioreactorState == eBioreactorState::CLEANING_CIRCULATION ||
                bioreactorState == eBioreactorState::CLEANING_RETURN ||
                bioreactorState == eBioreactorState::CLEANING_WAITING_WATER ||
                bioreactorState == eBioreactorState::CLEANING_WAITING_DETERGENT;
            if (state == eBioreactorState::IDLE && cleaningActive)
            {
                const eCleaningCycle stoppedCycle = cleaningCycle;
                const eBioreactorState stoppedPhase = bioreactorState;
                if (stopCleaningSequence())
                    Serial.println("OK:STATE=IDLE" + idSuffix);
                else
                    printCleaningCommandError(
                        "SAFE_OUTPUT_COMMAND_FAILED",
                        cleaningCycleName(stoppedCycle),
                        cleaningPhaseName(stoppedPhase),
                        hasId,
                        commandId);
                return;
            }

            if (isCleaningPhase(state))
            {
                printContextError(
                    "ERR:STATE:",
                    "STATE_CLEANING_REQUIRES_CYCLE_AND_PHASE",
                    cleaningCycleName(cleaningCycle),
                    cleaningPhaseName(state),
                    hasId,
                    commandId);
                return;
            }
            if (cleaningActive)
            {
                printCleaningCommandError(
                    "STOP_BEFORE_CHANGING_STATE",
                    cleaningCycleName(cleaningCycle),
                    cleaningPhaseName(bioreactorState),
                    hasId,
                    commandId);
                return;
            }

            setBioreactorState((uint8_t)state);
            Serial.println("OK:STATE=" + name + idSuffix);
            return;
        }

        float temp;
        char trailingCharacter;
        if (sscanf(command.c_str(), "TEMP=%f %c", &temp, &trailingCharacter) == 1 && isfinite(temp))
        {
            if (!setBaseTemperatureReference(temp))
            {
                Serial.println("ERR:TEMP:STORAGE" + idSuffix);
                return;
            }
            Serial.println("OK:" + command + idSuffix);
            Serial.print("Temperature Reference updated to: ");
            Serial.println(temp);
            return;
        }
        float ph;
        if (sscanf(command.c_str(), "PH=%f %c", &ph, &trailingCharacter) == 1 && isfinite(ph))
        {
            bioreactorParameter.putFloat("ph", ph);
            Serial.println("OK:" + command + idSuffix);
            Serial.print("pH Reference updated to: ");
            Serial.println(ph);
            return;
        }
        float oxy_percent;
        if (sscanf(command.c_str(), "DO=%f %c", &oxy_percent, &trailingCharacter) == 1 && isfinite(oxy_percent))
        {
            bioreactorParameter.putFloat("do", oxy_percent);
            Serial.println("OK:" + command + idSuffix);
            Serial.print("Dissolved Oxygen Reference updated to: ");
            Serial.println(oxy_percent);
            return;
        }
        float CO2_PPM;
        if (sscanf(command.c_str(), "CO2=%f %c", &CO2_PPM, &trailingCharacter) == 1 && isfinite(CO2_PPM))
        {
            bioreactorParameter.putFloat("CO2", CO2_PPM);
            pressureChamber.setReferenceLevel(CO2, CO2_PPM);
            Serial.println("OK:" + command + idSuffix);
            Serial.print("CO2 Reference Level updated to: ");
            Serial.println(CO2_PPM);
            return;
        }
        float O2_PPM;
        if (sscanf(command.c_str(), "O2=%f %c", &O2_PPM, &trailingCharacter) == 1 && isfinite(O2_PPM))
        {
            bioreactorParameter.putFloat("O2", O2_PPM);
            pressureChamber.setReferenceLevel(O2, O2_PPM);
            Serial.println("OK:" + command + idSuffix);
            Serial.print("O2 Reference Level updated to: ");
            Serial.println(O2_PPM);
            return;
        }
        float pumpSpeeds[4];
        if (sscanf(command.c_str(), "PUMP-SPEED=%f,%f,%f,%f %c",
                   &pumpSpeeds[0], &pumpSpeeds[1], &pumpSpeeds[2], &pumpSpeeds[3], &trailingCharacter) == 4 &&
            isfinite(pumpSpeeds[0]) && isfinite(pumpSpeeds[1]) &&
            isfinite(pumpSpeeds[2]) && isfinite(pumpSpeeds[3]))
        {
            printCommandError("UNSUPPORTED", hasId, commandId);
            return;
        }
        if (command == "CALIB-PH=4")
        {
            pHSensor.calibrateSinglePoint(eCalibrationValues::CAL_PH_4);
            Serial.println("OK:" + command + idSuffix);
            Serial.println("pH Sensor calibrated at pH 4");
            return;
        }
        if (command == "CALIB-PH=7")
        {
            pHSensor.calibrateSinglePoint(eCalibrationValues::CAL_PH_7);
            Serial.println("OK:" + command + idSuffix);
            Serial.println("pH Sensor calibrated at pH 7");
            return;
        }
        if (command == "CALIB-PH=10")
        {
            pHSensor.calibrateSinglePoint(eCalibrationValues::CAL_PH_10);
            Serial.println("OK:" + command + idSuffix);
            Serial.println("pH Sensor calibrated at pH 10");
            return;
        }
        if (command == "CALIB-DO=0" || command == "CALIB-DO=100")
        {
            printCommandError("UNSUPPORTED", hasId, commandId);
            return;
        }
        // Test state commands
        if (command == "STATE_TEST=TEST_INIT")
        {
            setBioreactorTestState((uint8_t)eTestState::TEST_INIT);
            Serial.println("OK:" + command + idSuffix);
            Serial.println("Bioreactor Test State set to TEST_INIT");
            return;
        }
        if (command == "STATE_TEST=TEST_STEP1_37C_10ML")
        {
            setBioreactorTestState((uint8_t)eTestState::TEST_STEP1_37C_10ML);
            Serial.println("OK:" + command + idSuffix);
            Serial.println("Bioreactor Test State set to TEST_STEP1_37C_10ML");
            return;
        }
        if (command == "STATE_TEST=COOL_DOWN")
        {
            setBioreactorTestState((uint8_t)eTestState::COOL_DOWN);
            Serial.println("OK:" + command + idSuffix);
            Serial.println("Bioreactor Test State set to COOL_DOWN");
            return;
        }
        if (command == "STATE_TEST=TEST_STEP2_37C_100ML")
        {
            setBioreactorTestState((uint8_t)eTestState::TEST_STEP2_37C_100ML);
            Serial.println("OK:" + command + idSuffix);
            Serial.println("Bioreactor Test State set to TEST_STEP2_37C_100ML");
            return;
        }

        if (command == "STATE_TEST=TEST_STEP3_RAMP")
        {
            setBioreactorTestState((uint8_t)eTestState::TEST_STEP3_RAMP);
            Serial.println("OK:" + command + idSuffix);
            Serial.println("Bioreactor Test State set to TEST_STEP3_RAMP");
            return;
        }
        if (command == "STATE_TEST=TEST_SET_37")
        {
            setBioreactorTestState((uint8_t)eTestState::TEST_SET_37);
            Serial.println("OK:" + command + idSuffix);
            Serial.println("Bioreactor Test State set to TEST_SET_37");
            return;
        }
        if (command == "STATE_TEST=TEST_SET_37_2")
        {
            setBioreactorTestState((uint8_t)eTestState::TEST_SET_37);
            setBioreacteurCompletedTest(0x07);
            Serial.println("OK:" + command + idSuffix);
            Serial.println("Bioreactor Test State set to TEST_SET_37");
            return;
        }
        if (command == "STATE_TEST=TEST_STEP4_37_5")
        {
            setBioreactorTestState((uint8_t)eTestState::TEST_STEP4_37_5);
            setBioreacteurCompletedTest(0x00);
            Serial.println("OK:" + command + idSuffix);
            Serial.println("Bioreactor Test State set to TEST_STEP4_37_5");
            return;
        }
        if (command == "STATE_TEST=TEST_STEP5_36_5")
        {
            setBioreactorTestState((uint8_t)eTestState::TEST_STEP5_36_5);
            setBioreacteurCompletedTest(0x07);
            Serial.println("OK:" + command + idSuffix);
            Serial.println("Bioreactor Test State set to TEST_STEP5_36_5");
            return;
        }

        uint32_t amax;
        if (sscanf(command.c_str(), "AMAX=%lu %c", &amax, &trailingCharacter) == 1)
        {
            approvPump.setAMAX(amax);
            circulationPump.setAMAX(amax);
            cultureChamberPump1.setAMAX(amax);
            cultureChamberPump2.setAMAX(amax);
            Serial.println("OK:" + command + idSuffix);
            Serial.println("AMAX Set : " + String(amax));
            return;
        }

        printCommandError("UNKNOWN", hasId, commandId);
    }
}