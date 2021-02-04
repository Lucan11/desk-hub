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
    temperature_sensor_data_t sensor_values_inside, sensor_values_outside;
    unsigned int i = 0;

    // Initialize all modules
    init();

    // Enter main loop.
    while(true) {
        idle_state_handle();

        // Make this a timer later
        if(i % 10 == 0) {
            sensor_values_inside = temperature_sensor_read();
            sensor_values_outside = bluetooth_get_outside_temperature();

            NRF_LOG_INFO("Temp inside: %i outside: %i", sensor_values_inside.temperature, sensor_values_outside.temperature);
            NRF_LOG_INFO("Humidity inside: %i outside: %i", sensor_values_inside.humidity, sensor_values_outside.humidity);
        }

        i++;
    }
}
