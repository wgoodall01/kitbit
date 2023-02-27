#include "watchdog.h"
#include "nrf_wdt.h"

// Global instance of the watchdog timer.
class Watchdog Watchdog;

bool Watchdog::enable(int timeout_ms) {
    // If the timeout is zero, we're good.
    if (timeout_ms <= 0)
        return true;

    // If the watchdog has already been started, return false.
    if (nrf_wdt_started())
        return false;

    // WDT run when CPU is sleep
    nrf_wdt_behaviour_set(NRF_WDT_BEHAVIOUR_RUN_SLEEP);
    nrf_wdt_reload_value_set((timeout_ms * 32768) / 1000);

    // Use channel zero.
    nrf_wdt_reload_request_enable(NRF_WDT_RR0);

    // Start the watchdog timer.
    nrf_wdt_task_trigger(NRF_WDT_TASK_START);
}

void Watchdog::reset() {
    nrf_wdt_reload_request_set(NRF_WDT_RR0);
}
