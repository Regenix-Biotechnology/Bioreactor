#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <Arduino.h>
#include <esp_task_wdt.h>

constexpr uint8_t WATCHDOG_TIMER = 60;

void initWatchDog(uint8_t watchdogTime);
void kickWatchDog();

#endif // WATCHDOG_H