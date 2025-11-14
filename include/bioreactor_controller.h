#ifndef BIOREACTOR_CONTROLLER_H
#define BIOREACTOR_CONTROLLER_H

#include "main.h"

/**
 * This file contains the functions that are called in the main loop to control the bioreactor.
 */

void beginBioreactorController();
void setFansState(bool heaterFanState, bool circulationFanState, bool rightFanState, bool leftFanState, bool pcbFanState, bool lowVoltFanState, bool highvoltFanState);
void setValvesState(bool valveSupplyState, bool valveCirculationState, bool valveReturnState);
void setPressureChamberValvesState(bool o2ValveState, bool co2ValveState, bool airValveState);
void setPressureChamberState(bool state);
void setPumpsSpeed(float approvPumpSpeed, float circulationPumpSpeed, float cultureChamberPump1Speed, float cultureChamberPump2Speed);
void setHeatersState(float heaterState);
void updateTemperatureController();
void updatePressureChamberController();
void updateSensors();
void printBioreactorStateToSerial();

#endif
