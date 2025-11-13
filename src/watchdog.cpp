#include "watchdog.h"

void initWatchDog(uint8_t watchdogTimer = WATCHDOG_TIMER){
    esp_task_wdt_init(watchdogTimer, true); // Enable panic (reset)
    esp_task_wdt_add(NULL); // Add current thread (loopTask)
}

void kickWatchDog(){
    esp_task_wdt_reset();
}