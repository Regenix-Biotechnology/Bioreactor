#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include "SHT40.h"
#include "Pyroscience.h"
#include "Pump.h"
#include "ssr_relay.h"
#include "relay.h"
#include "pins.h"

enum class eBioreactorState
{
    IDLE = 0,
    APPROV,
    PREPARE,
    RUN,

    TEST,
    MAX_STATES
};

// Function prototypes
void closeAllRelay();
void closeAllPumps();

// Global variables
eBioreactorState bioreactorState = eBioreactorState::IDLE;

#endif