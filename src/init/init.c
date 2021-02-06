#include "init.h"

#include "bsp.h"
#include "app_timer.h"
#include "nrf_pwr_mgmt.h"

#include "bluetooth.h"
#include "log.h"
#include "Si7021.h"
#include "ST7735.h"



static void leds_init(void)
{
    ret_code_t err_code = bsp_init(BSP_INIT_LEDS, NULL);
    APP_ERROR_CHECK(err_code);
}


static void timers_init(void)
{
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
}


static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}


void init(){
    log_init();
    timers_init();
    leds_init();
    power_management_init();

    // The bluetooth has the tendency to hardfault very often
    // To make sure we can see that it has actually finished, log before and after
    NRF_LOG_INFO("init bluetooth");
    log_flush();
    bluetooth_init();
    NRF_LOG_INFO("init bluetooth done");
    log_flush();

    temperature_sensor_init();
    display_init();

    NRF_LOG_INFO("Desk hub initialized");
}
