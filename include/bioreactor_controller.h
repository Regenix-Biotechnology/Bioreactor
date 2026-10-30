#ifndef BIOREACTOR_CONTROLLER_H
#define BIOREACTOR_CONTROLLER_H

#include "main.h"

struct CultureSetpoints
{
    float temperatureMin;
    float temperatureMax;
    float circulationPumpSpeed;
    float culturePump1Speed;
    float culturePump2Speed;
};

enum class eCultureSetpointResult : unsigned char
{
    ACCEPTED = 0,
    INVALID_STATE,
    INVALID_VALUES,
    STORAGE_FAILURE
};

/**
 * This file contains the functions that are called in the main loop to control the bioreactor.
 */

void beginBioreactorController();
bool setFansState(bool heaterFanState, bool circulationFanState, bool rightFanState, bool leftFanState, bool pcbFanState, bool lowVoltFanState, bool highvoltFanState);
bool setValvesState(bool valveSupplyState, bool valveCirculationState, bool valveReturnState);
bool setPressureChamberValvesState(bool o2ValveState, bool co2ValveState, bool airValveState, bool co2AirValveState);
void setPressureChamberState(bool state);
bool setPumpsSpeed(float approvPumpSpeed, float circulationPumpSpeed, float cultureChamberPump1Speed, float cultureChamberPump2Speed);
void setHeatersState(bool heaterState);
bool setCleaningSafeOutputs();
bool isCleaningTemperatureSafe();
void failCleaningSequence(const char *reason);
void failCleaningSequence(const char *reason, eCleaningCycle cycle, eBioreactorState phase);
void reportCleaningStartFailure(const char *reason, eCleaningCycle cycle, eBioreactorState phase);
bool stopCleaningSequence();
const char *getLastCleaningErrorCode();
void updateTemperatureController();
void updatePressureChamberController();
void updateSensors();
void printBioreactorStateToSerial();
void updateLEDState();
void setBioreactorState(uint8_t state);
bool isBioreactorStateSupported(eBioreactorState state);
eCleaningCommandResult setCleaningCyclePhase(eCleaningCycle cycle, eBioreactorState phase);
void advanceCleaningPhase(eBioreactorState phase);
void finishCleaningCycle();
void receiveSerialCommand();
void beginBioreactorPreferences();
void updateCO2Controller();
eCultureSetpointResult stageCultureSetpoints(const CultureSetpoints &setpoints);
bool setBaseTemperatureReference(float temperature);
bool getRunCulturePumpSpeeds(float &circulationSpeed, float &culturePump1Speed, float &culturePump2Speed);
void applyCultureTemperatureForCurrentState();

#endif
