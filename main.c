#include "nrf_pwr_mgmt.h"

#include "app_timer.h"
#include "nrf_drv_clock.h"

#include "log.h"
#include "init.h"
#include "Si7021.h"
#include "bluetooth.h"



APP_TIMER_DEF(m_sensor_timer);

static volatile unsigned int reading = 0;
static void sensor_timer_handler(void * p_context) {
    // We cannot read the sensor from here, I suspect this is because
    // this handler is called from an interrupt and the I2C also uses
    // interrupts at the same (or lower) priority.
    // This could thus be fixed by changing the priorities, but I'd
    // rather not read the I2C from an interrupt anyway
    reading = 1;
}

static inline void idle_state_handle(void) {
    // Process log entries if there are any, then sleep
    log_flush();

    nrf_pwr_mgmt_run();
}


int main(void) {
    ret_code_t err_code;
    temperature_sensor_data_t sensor_values_inside, sensor_values_outside;

    // Initialize all modules
    init();

    // Create timer
    err_code = app_timer_create(&m_sensor_timer,
                                APP_TIMER_MODE_REPEATED,
                                sensor_timer_handler);
    APP_ERROR_CHECK(err_code);

    // Start timer
    err_code = app_timer_start(m_sensor_timer, APP_TIMER_TICKS(1000), NULL);
    APP_ERROR_CHECK(err_code);

    // Enter main loop.
    while(true) {
        idle_state_handle();

        if(reading == 1) {
            sensor_values_inside = temperature_sensor_read();
            sensor_values_outside = bluetooth_get_outside_temperature();

            NRF_LOG_INFO("Temp inside: %i outside: %i", sensor_values_inside.temperature, sensor_values_outside.temperature);
            NRF_LOG_INFO("Humidity inside: %i outside: %i", sensor_values_inside.humidity, sensor_values_outside.humidity);

            reading = 0;
        }
    }
}
