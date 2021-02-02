#include "Si7021.h"

#include "app_error.h"

#include "nrfx_twi.h"

#include "log.h"

static const uint8_t address = 0x40;

// Create an TWI instance using register 0
static const nrfx_twi_t instance = NRFX_TWI_INSTANCE(0);
// static volatile unsigned int done = 0;


// static void twi_handler(nrfx_twi_evt_t const * p_event, void * p_context)
// {
//     // done = 1;
//     NRF_LOG_INFO("interrupt");
//     log_flush();
// }


void temperature_sensor_init() {
    nrfx_err_t res;
    const uint8_t reset[1] = {0xFE};

    // Set the configuration
    const nrfx_twi_config_t config = {                                                \
        .frequency          = NRF_TWI_FREQ_100K,                                      \
        .scl                = 12,                                                     \
        .sda                = 11,                                                     \
        .interrupt_priority = NRFX_TWI_DEFAULT_CONFIG_IRQ_PRIORITY,                   \
        .hold_bus_uninit    = NRFX_TWI_DEFAULT_CONFIG_HOLD_BUS_UNINIT,                \
    };

    // Blocking mode, so no event handler (context) needed
    res = nrfx_twi_init(&instance, &config, NULL, NULL);
    NRF_LOG_INFO("init result:");
    APP_ERROR_CHECK(res);

    nrfx_twi_enable(&instance);

    // Send reset command
    res = nrfx_twi_tx(&instance, address, reset, sizeof(reset), 0);
    APP_ERROR_CHECK(res);
}

void temperature_sensor_read() {
    nrfx_err_t res;
    uint8_t USR1[1] = {0xE7};
    uint8_t RX[1] = {0};

    res = nrfx_twi_tx(&instance, address, USR1, sizeof(USR1), 1);
    APP_ERROR_CHECK(res);
    res = nrfx_twi_rx(&instance, address, RX, 1);
    APP_ERROR_CHECK(res);

    NRF_LOG_INFO("read RX0: %i", RX[0]);

    // const nrfx_twi_xfer_desc_t desc = NRFX_TWI_XFER_DESC_TXRX(address, USR1, 1, RX, 1);
    // res = nrfx_twi_xfer(&instance, &desc, NRFX_TWI_FLAG_TX_NO_STOP);
    // APP_ERROR_CHECK(res);

    // while(done == 0);

    NRF_LOG_INFO("read RX0: %i", RX[0]);
}
