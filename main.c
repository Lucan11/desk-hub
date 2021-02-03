#include "nrf_pwr_mgmt.h"

#include "log.h"
#include "init.h"
#include "Si7021.h"
#include "bluetooth.h"


static void idle_state_handle(void)
{
    // Process log entries if there are any, otherwise sleep
    if (log_flush() == false)
    {
        nrf_pwr_mgmt_run();
    }
}


int main(void) {
    temperature_sensor_data_t current_sensor_values;
    unsigned int i = 0;

    // Initialize all modules
    init();

    // Enter main loop.
    while(true) {
        idle_state_handle();

        // Make this a timer later
        if(i % 5 == 0) {
            current_sensor_values = temperature_sensor_read();
            bluetooth_update_advertisement_data(&current_sensor_values);
        }

        i++;
    }
}
