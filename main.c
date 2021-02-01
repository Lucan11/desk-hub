

#include "nrf_pwr_mgmt.h"

#include "log.h"
#include "init.h"


static void idle_state_handle(void)
{
    // Process log entries if there are any, otherwise sleep
    if ( log_flush() == false)
    {
        nrf_pwr_mgmt_run();
    }
}


int main(void) {

    // Initialize all modules
    init();

    NRF_LOG_INFO("testlog!");

    // Enter main loop.
    while(true) {
        idle_state_handle();
    }
}
