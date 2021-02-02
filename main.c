#include "nrf_pwr_mgmt.h"

#include "log.h"
#include "init.h"
#include "Si7021.h"


static void idle_state_handle(void)
{
    // Process log entries if there are any, otherwise sleep
    if (log_flush() == false)
    {
        nrf_pwr_mgmt_run();
    }
}


int main(void) {
    unsigned int i = 0;

    // Initialize all modules
    init();

    // Enter main loop.
    while(true) {
        idle_state_handle();

        if(i % 5 == 0)
            temperature_sensor_read();

        i++;
    }
}
