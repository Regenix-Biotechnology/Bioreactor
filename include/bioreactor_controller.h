#ifndef BIOREACTOR_CONTROLLER_H
#define BIOREACTOR_CONTROLLER_H

#include <main.h>

/**
 * This file contains the functions that are called in the main loop to control the bioreactor.
 */

void beginBioreactorController();
void setFansState(bool heaterFanState, bool interiorFanState, bool exteriorFanState);
void setValvesState(bool valve1State, bool valve2State, bool valve3State, bool valve4State, bool valve5State);
void setPressureChamberValvesState(bool o2ValveState, bool co2ValveState, bool airValveState, bool safetyValveState);
void setState(bool state);
void setPumpsSpeed(uint8_t approvPumpSpeed, uint8_t sensorPumpSpeed, uint8_t cultureChamberPump1Speed, uint8_t cultureChamberPump2Speed);
void setHeatersState(float heaterState, bool patchHeaterState);
void updateTemperatureController();
void updatePressureChamberController();

#endif
