#include <cassert>
#include "cleaning_sequence.h"
#include "serial_protocol.h"

int main()
{
    assert(static_cast<int>(eBioreactorState::IDLE) == 0);
    assert(static_cast<int>(eBioreactorState::APPROV) == 1);
    assert(static_cast<int>(eBioreactorState::CLEANING_APPROV) == 5);
    assert(static_cast<int>(eBioreactorState::CLEANING_CIRCULATION) == 6);
    assert(static_cast<int>(eBioreactorState::CLEANING_RETURN) == 7);
    assert(static_cast<int>(eBioreactorState::TEST) == 13);
    assert(static_cast<int>(eBioreactorState::TRASH) == 17);
    assert(static_cast<int>(eBioreactorState::MEDIA_CHANGE_TRASH) == 18);
    assert(static_cast<int>(eBioreactorState::MEDIA_CHANGE_APPROV) == 19);
    assert(static_cast<int>(eBioreactorState::CLEANING_WAITING_WATER) == 20);
    assert(static_cast<int>(eBioreactorState::CLEANING_WAITING_DETERGENT) == 21);
    assert(static_cast<int>(eBioreactorState::ASSEMBLY) == 22);

    // Order: water pre-rinse, detergent, water rinse (its RETURN empties the circuit)
    assert(isValidCleaningOperatorCommand(
        eBioreactorState::IDLE,
        eCleaningCycle::NONE,
        eCleaningCycle::PRE_RINSE,
        eBioreactorState::CLEANING_WAITING_WATER));
    assert(isValidCleaningOperatorCommand(
        eBioreactorState::CLEANING_WAITING_WATER,
        eCleaningCycle::PRE_RINSE,
        eCleaningCycle::RINSE,
        eBioreactorState::CLEANING_APPROV));
    assert(isValidCleaningOperatorCommand(
        eBioreactorState::CLEANING_WAITING_WATER,
        eCleaningCycle::PRE_RINSE,
        eCleaningCycle::PRE_RINSE,
        eBioreactorState::CLEANING_APPROV));
    // The detergent can no longer be the first step
    assert(!isValidCleaningOperatorCommand(
        eBioreactorState::IDLE,
        eCleaningCycle::NONE,
        eCleaningCycle::DETERGENT,
        eBioreactorState::CLEANING_WAITING_DETERGENT));
    assert(!isValidCleaningOperatorCommand(
        eBioreactorState::CLEANING_WAITING_WATER,
        eCleaningCycle::PRE_RINSE,
        eCleaningCycle::DETERGENT,
        eBioreactorState::CLEANING_APPROV));
    assert(nextCleaningStateAfterCycle(eCleaningCycle::PRE_RINSE) ==
           eBioreactorState::CLEANING_WAITING_DETERGENT);
    assert(isValidCleaningPhaseTransition(
        eBioreactorState::CLEANING_APPROV,
        eCleaningCycle::PRE_RINSE,
        eBioreactorState::CLEANING_CIRCULATION));
    assert(!isHeaterPermitted(eBioreactorState::CLEANING_APPROV, eCleaningCycle::PRE_RINSE, true));
    assert(isValidCleaningOperatorCommand(
        eBioreactorState::CLEANING_WAITING_DETERGENT,
        eCleaningCycle::DETERGENT,
        eCleaningCycle::DETERGENT,
        eBioreactorState::CLEANING_APPROV));
    assert(isValidCleaningOperatorCommand(
        eBioreactorState::CLEANING_WAITING_WATER,
        eCleaningCycle::DETERGENT,
        eCleaningCycle::RINSE,
        eBioreactorState::CLEANING_APPROV));

    assert(!isValidCleaningOperatorCommand(
        eBioreactorState::IDLE,
        eCleaningCycle::NONE,
        eCleaningCycle::DETERGENT,
        eBioreactorState::CLEANING_APPROV));
    assert(!isValidCleaningOperatorCommand(
        eBioreactorState::IDLE,
        eCleaningCycle::NONE,
        eCleaningCycle::RINSE,
        eBioreactorState::CLEANING_APPROV));
    assert(!isValidCleaningOperatorCommand(
        eBioreactorState::CLEANING_APPROV,
        eCleaningCycle::DETERGENT,
        eCleaningCycle::RINSE,
        eBioreactorState::CLEANING_APPROV));
    assert(!isValidCleaningOperatorCommand(
        eBioreactorState::CLEANING_WAITING_WATER,
        eCleaningCycle::DETERGENT,
        eCleaningCycle::PRE_RINSE,
        eBioreactorState::CLEANING_APPROV));
    assert(!isValidCleaningOperatorCommand(
        eBioreactorState::IDLE,
        eCleaningCycle::NONE,
        eCleaningCycle::DETERGENT,
        eBioreactorState::CLEANING_CIRCULATION));

    const eCleaningCycle cycles[] = {
        eCleaningCycle::DETERGENT,
        eCleaningCycle::RINSE,
        eCleaningCycle::PRE_RINSE};
    for (eCleaningCycle cycle : cycles)
    {
        assert(isValidCleaningPhaseTransition(
            eBioreactorState::CLEANING_APPROV,
            cycle,
            eBioreactorState::CLEANING_CIRCULATION));
        assert(isValidCleaningPhaseTransition(
            eBioreactorState::CLEANING_CIRCULATION,
            cycle,
            eBioreactorState::CLEANING_RETURN));
        assert(!isValidCleaningPhaseTransition(
            eBioreactorState::CLEANING_APPROV,
            cycle,
            eBioreactorState::CLEANING_RETURN));
        assert(!isValidCleaningPhaseTransition(
            eBioreactorState::CLEANING_RETURN,
            cycle,
            eBioreactorState::CLEANING_CIRCULATION));
    }

    assert(!isValidCleaningPhaseTransition(
        eBioreactorState::CLEANING_APPROV,
        eCleaningCycle::NONE,
        eBioreactorState::CLEANING_CIRCULATION));
    assert(isCleaningManagedState(eBioreactorState::CLEANING_WAITING_WATER));
    assert(isCleaningManagedState(eBioreactorState::CLEANING_WAITING_DETERGENT));
    assert(!isCleaningManagedState(eBioreactorState::IDLE));
    assert(getSafeStartupState(eBioreactorState::CLEANING_APPROV) == eBioreactorState::IDLE);
    assert(getSafeStartupState(eBioreactorState::CLEANING_CIRCULATION) == eBioreactorState::IDLE);
    assert(getSafeStartupState(eBioreactorState::CLEANING_RETURN) == eBioreactorState::IDLE);
    assert(getSafeStartupState(eBioreactorState::CLEANING_WAITING_WATER) == eBioreactorState::IDLE);
    assert(getSafeStartupState(eBioreactorState::CLEANING_WAITING_DETERGENT) == eBioreactorState::IDLE);
    assert(getSafeStartupState(eBioreactorState::RUN) == eBioreactorState::RUN);
    assert(nextCleaningCycle(eCleaningCycle::DETERGENT) == eCleaningCycle::DETERGENT);
    assert(nextCleaningStateAfterCycle(eCleaningCycle::DETERGENT) ==
           eBioreactorState::CLEANING_WAITING_WATER);
    assert(nextCleaningCycle(eCleaningCycle::RINSE) == eCleaningCycle::NONE);
    assert(nextCleaningStateAfterCycle(eCleaningCycle::RINSE) == eBioreactorState::IDLE);
    assert(getSafeStartupState(eBioreactorState::ASSEMBLY) == eBioreactorState::ASSEMBLY);

    assert(isCleaningManagedState(eBioreactorState::CLEANING_APPROV));
    assert(isCleaningManagedState(eBioreactorState::CLEANING_CIRCULATION));
    assert(isCleaningManagedState(eBioreactorState::CLEANING_RETURN));
    assert(isCleaningManagedState(eBioreactorState::CLEANING_WAITING_WATER));
    assert(canStopCleaningSequence(eBioreactorState::CLEANING_APPROV));
    assert(canStopCleaningSequence(eBioreactorState::CLEANING_CIRCULATION));
    assert(canStopCleaningSequence(eBioreactorState::CLEANING_RETURN));
    assert(canStopCleaningSequence(eBioreactorState::CLEANING_WAITING_WATER));
    assert(canStopCleaningSequence(eBioreactorState::CLEANING_WAITING_DETERGENT));
    assert(!canStopCleaningSequence(eBioreactorState::IDLE));
    assert(isWaitingForWater(eBioreactorState::CLEANING_WAITING_WATER));
    assert(isWaitingForDetergent(eBioreactorState::CLEANING_WAITING_DETERGENT));
    assert(!isWaitingForWater(eBioreactorState::IDLE));
    assert(!isWaitingForDetergent(eBioreactorState::IDLE));

    size_t commandLength = 0;
    bool hasId = false;
    uint32_t commandId = 0;
    const char commandWithId[] = "CLEANING=RINSE,APPROV;ID=42";
    assert(extractCommandId(
        commandWithId,
        sizeof(commandWithId) - 1,
        commandLength,
        hasId,
        commandId));
    assert(hasId && commandId == 42);
    assert(commandLength == sizeof("CLEANING=RINSE,APPROV") - 1);
    assert(commandWithId[commandLength] == ';');

    const char commandWithoutId[] = "CLEANING=DETERGENT,APPROV";
    assert(extractCommandId(
        commandWithoutId,
        sizeof(commandWithoutId) - 1,
        commandLength,
        hasId,
        commandId));
    assert(!hasId && commandId == 0);
    assert(commandLength == sizeof(commandWithoutId) - 1);

    const char invalidId[] = "CLEANING=DETERGENT,APPROV;ID=abc";
    assert(!extractCommandId(invalidId, sizeof(invalidId) - 1, commandLength, hasId, commandId));
    const char overflowingId[] = "STATUS?;ID=42949672960";
    assert(!extractCommandId(
        overflowingId,
        sizeof(overflowingId) - 1,
        commandLength,
        hasId,
        commandId));
    const char statusWithId[] = "STATUS?;ID=7";
    assert(extractCommandId(
        statusWithId,
        sizeof(statusWithId) - 1,
        commandLength,
        hasId,
        commandId));
    assert(hasId && commandId == 7);
    assert(isStatusQuery(statusWithId, commandLength));
    assert(!isStatusQuery("STATUS?x", sizeof("STATUS?x") - 1));

    const eBioreactorState productionHeatingStates[] = {
        eBioreactorState::APPROV,
        eBioreactorState::PREPARE,
        eBioreactorState::RUN,
        eBioreactorState::MEDIA_CHANGE_TRASH,
        eBioreactorState::MEDIA_CHANGE_APPROV};
    for (eBioreactorState state : productionHeatingStates)
        assert(isHeaterPermitted(state, eCleaningCycle::NONE, false));

    // END return empties the circuit, assembly keeps everything off
    assert(!isHeaterPermitted(eBioreactorState::CELL_RETURN, eCleaningCycle::NONE, true));
    assert(!isHeaterPermitted(eBioreactorState::ASSEMBLY, eCleaningCycle::NONE, true));
    assert(!isHeaterPermitted(eBioreactorState::TEST, eCleaningCycle::NONE, false));
    assert(isHeaterPermitted(eBioreactorState::TEST, eCleaningCycle::NONE, false, true));

    const eBioreactorState cleaningPhases[] = {
        eBioreactorState::CLEANING_APPROV,
        eBioreactorState::CLEANING_CIRCULATION,
        eBioreactorState::CLEANING_RETURN};
    for (eBioreactorState state : cleaningPhases)
    {
        assert(isHeaterPermitted(state, eCleaningCycle::DETERGENT, true));
        assert(!isHeaterPermitted(state, eCleaningCycle::DETERGENT, false));
        assert(!isHeaterPermitted(state, eCleaningCycle::RINSE, true));
        assert(!isHeaterPermitted(state, eCleaningCycle::PRE_RINSE, true));
    }
    assert(!isHeaterPermitted(
        eBioreactorState::CLEANING_WAITING_WATER,
        eCleaningCycle::DETERGENT,
        true));
    assert(!isHeaterPermitted(
        eBioreactorState::CLEANING_WAITING_DETERGENT,
        eCleaningCycle::DETERGENT,
        true));
    return 0;
}
