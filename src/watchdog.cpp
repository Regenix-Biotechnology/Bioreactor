#include "watchdog.h"

/**
 * @brief Initialise the watchdog must be done at the end of setup for it not to reset during the setup
 *
 */
void initWatchDog()
{
    esp_task_wdt_init(WATCHDOG_TIMER, true); // Enable panic (reset)
    esp_task_wdt_add(NULL);                  // Add current thread (loopTask)
}

void kickWatchDog()
{
    esp_task_wdt_reset();
}