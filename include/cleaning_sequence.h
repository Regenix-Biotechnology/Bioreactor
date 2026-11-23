#ifndef CLEANING_SEQUENCE_H
#define CLEANING_SEQUENCE_H

enum class eBioreactorState
{
    IDLE = 0,
    APPROV = 1,
    PREPARE = 2,
    RUN = 3,
    CELL_RETURN = 4,
    CLEANING_APPROV = 5,
    CLEANING_CIRCULATION = 6,
    CLEANING_RETURN = 7,
    TEST = 13,
    TRASH = 17,
    MEDIA_CHANGE_TRASH = 18,
    MEDIA_CHANGE_APPROV = 19,
    CLEANING_WAITING_WATER = 20,
    CLEANING_WAITING_DETERGENT = 21,
    ASSEMBLY = 22, // assembly / disassembly (autoclave): all process valves open, everything else off
    MAX_STATE = 23
};

/**
 * Cleaning sequence (2026-11-23):
 *   IDLE --CLEANING=PRE_RINSE,WAITING_FOR_WATER--> WAITING_FOR_WATER (cycle PRE_RINSE)
 *        --CLEANING=RINSE,APPROV (operator: water added)--> PRE_RINSE: APPROV -> CIRCULATION -> RETURN
 *        --> WAITING_FOR_DETERGENT (cycle DETERGENT)
 *        --CLEANING=DETERGENT,APPROV (operator: detergent added)--> DETERGENT: APPROV -> CIRCULATION -> RETURN
 *        --> WAITING_FOR_WATER (cycle DETERGENT)
 *        --CLEANING=RINSE,APPROV (operator: water added)--> RINSE: APPROV -> CIRCULATION -> RETURN
 *        --> IDLE (EVENT:CLEANING:COMPLETE)
 * The final RINSE return empties the circuit: there is no separate drying cycle anymore.
 */
enum class eCleaningCycle : unsigned char
{
    NONE = 0,
    DETERGENT,
    RINSE,
    PRE_RINSE // water rinse before the detergent
};

enum class eCleaningCommandResult : unsigned char
{
    ACCEPTED = 0,
    INVALID_TRANSITION,
    ACTUATOR_FAILURE,
    TEMPERATURE_SENSOR_NOT_READY
};

inline bool isCleaningPhaseState(eBioreactorState state)
{
    return state == eBioreactorState::CLEANING_APPROV ||
           state == eBioreactorState::CLEANING_CIRCULATION ||
           state == eBioreactorState::CLEANING_RETURN;
}

inline bool isCleaningManagedState(eBioreactorState state)
{
    return isCleaningPhaseState(state) ||
           state == eBioreactorState::CLEANING_WAITING_WATER ||
           state == eBioreactorState::CLEANING_WAITING_DETERGENT;
}

inline bool isWaitingForWater(eBioreactorState state)
{
    return state == eBioreactorState::CLEANING_WAITING_WATER;
}

inline bool isWaitingForDetergent(eBioreactorState state)
{
    return state == eBioreactorState::CLEANING_WAITING_DETERGENT;
}

inline bool canStopCleaningSequence(eBioreactorState state)
{
    return isCleaningManagedState(state);
}

inline const char *cleaningCycleName(eCleaningCycle cycle)
{
    switch (cycle)
    {
    case eCleaningCycle::DETERGENT:
        return "DETERGENT";
    case eCleaningCycle::RINSE:
        return "RINSE";
    case eCleaningCycle::PRE_RINSE:
        return "PRE_RINSE";
    default:
        return "NONE";
    }
}

inline const char *cleaningPhaseName(eBioreactorState state)
{
    switch (state)
    {
    case eBioreactorState::CLEANING_APPROV:
        return "APPROV";
    case eBioreactorState::CLEANING_CIRCULATION:
        return "CIRCULATION";
    case eBioreactorState::CLEANING_RETURN:
        return "RETURN";
    case eBioreactorState::CLEANING_WAITING_WATER:
        return "WAITING_FOR_WATER";
    case eBioreactorState::CLEANING_WAITING_DETERGENT:
        return "WAITING_FOR_DETERGENT";
    default:
        return "NONE";
    }
}

inline eCleaningCycle nextCleaningCycle(eCleaningCycle cycle)
{
    switch (cycle)
    {
    case eCleaningCycle::DETERGENT:
        return eCleaningCycle::DETERGENT;
    default:
        return eCleaningCycle::NONE;
    }
}

inline eBioreactorState nextCleaningStateAfterCycle(eCleaningCycle cycle)
{
    switch (cycle)
    {
    case eCleaningCycle::PRE_RINSE:
        return eBioreactorState::CLEANING_WAITING_DETERGENT;
    case eCleaningCycle::DETERGENT:
        return eBioreactorState::CLEANING_WAITING_WATER;
    default:
        return eBioreactorState::IDLE;
    }
}

inline eBioreactorState getSafeStartupState(eBioreactorState storedState)
{
    return isCleaningManagedState(storedState) ? eBioreactorState::IDLE : storedState;
}

inline bool isValidCleaningOperatorCommand(
    eBioreactorState currentState,
    eCleaningCycle currentCycle,
    eCleaningCycle requestedCycle,
    eBioreactorState requestedPhase)
{
    // Start of the sequence: wait for the water of the first rinse
    return (requestedCycle == eCleaningCycle::PRE_RINSE &&
            requestedPhase == eBioreactorState::CLEANING_WAITING_WATER &&
            currentState == eBioreactorState::IDLE &&
            currentCycle == eCleaningCycle::NONE) ||
           // Water added for the first rinse (RINSE,APPROV is accepted like PRE_RINSE,APPROV)
           ((requestedCycle == eCleaningCycle::PRE_RINSE || requestedCycle == eCleaningCycle::RINSE) &&
            requestedPhase == eBioreactorState::CLEANING_APPROV &&
            currentState == eBioreactorState::CLEANING_WAITING_WATER &&
            currentCycle == eCleaningCycle::PRE_RINSE) ||
           // Detergent added (after the first rinse)
           (requestedCycle == eCleaningCycle::DETERGENT &&
            requestedPhase == eBioreactorState::CLEANING_APPROV &&
            currentState == eBioreactorState::CLEANING_WAITING_DETERGENT &&
            currentCycle == eCleaningCycle::DETERGENT) ||
           (requestedCycle == eCleaningCycle::RINSE &&
            requestedPhase == eBioreactorState::CLEANING_APPROV &&
            currentState == eBioreactorState::CLEANING_WAITING_WATER &&
            currentCycle == eCleaningCycle::DETERGENT);
}

inline bool isValidCleaningPhaseTransition(
    eBioreactorState currentState,
    eCleaningCycle currentCycle,
    eBioreactorState nextPhase)
{
    const bool validCycle =
        currentCycle == eCleaningCycle::PRE_RINSE ||
        currentCycle == eCleaningCycle::DETERGENT ||
        currentCycle == eCleaningCycle::RINSE;
    if (!validCycle)
        return false;

    return (currentState == eBioreactorState::CLEANING_APPROV &&
            nextPhase == eBioreactorState::CLEANING_CIRCULATION) ||
           (currentState == eBioreactorState::CLEANING_CIRCULATION &&
            nextPhase == eBioreactorState::CLEANING_RETURN);
}

inline bool isHeaterPermitted(
    eBioreactorState state,
    eCleaningCycle cycle,
    bool cleaningTemperatureSafe,
    bool conditionTestActive = false)
{
    if (state == eBioreactorState::TEST)
        return conditionTestActive;

    if (state == eBioreactorState::APPROV ||
        state == eBioreactorState::PREPARE ||
        state == eBioreactorState::RUN ||
        state == eBioreactorState::MEDIA_CHANGE_TRASH ||
        state == eBioreactorState::MEDIA_CHANGE_APPROV)
        return true;

    return cycle == eCleaningCycle::DETERGENT &&
           isCleaningPhaseState(state) &&
           cleaningTemperatureSafe;
}

#endif
