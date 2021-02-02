#include "Si7021.h"

#include "app_error.h"
#include "nrfx_twi.h"
#include "nrf_delay.h"
#include <nrf_assert.h>
#include "log.h"

// Device specific commands
static const uint8_t power_up_time_ms = 15;
static const uint8_t address = 0x40;
static const uint8_t RST_REG = 0xFE;
static const uint8_t USR_REG = 0xE7;
static const uint8_t USR_RES0 = 0;
static const uint8_t USR_RES1 = 7;

static const nrfx_twi_t instance = NRFX_TWI_INSTANCE(0);    // Create an TWI instance using register 0
static volatile unsigned int transfer_done = 0;

static inline void wait_for_transfer() {
    while(transfer_done == 0) {
        // Maybe go to sleep here?
    }

    transfer_done = 0;
}

static inline void reset() {
    nrfx_err_t res;

    res = nrfx_twi_tx(&instance, address, &RST_REG, 1, 0);
    APP_ERROR_CHECK(res);

    wait_for_transfer();

    nrf_delay_ms(power_up_time_ms);
}

static void twi_handler(nrfx_twi_evt_t const * p_event, void * p_context) {
    nrfx_twi_0_irq_handler();
    transfer_done = 1;
}

static inline uint8_t read_byte(const uint8_t reg) {
    nrfx_err_t res;
    uint8_t RX[1] = {0};
    const nrfx_twi_xfer_desc_t desc = NRFX_TWI_XFER_DESC_TXRX(address, (uint8_t*)&reg, 1, RX, 1);

    res = nrfx_twi_xfer(&instance, &desc, NRFX_TWI_FLAG_TX_NO_STOP);
    APP_ERROR_CHECK(res);

    wait_for_transfer();

    return RX[0];
}

static inline void write_byte(const uint8_t reg, const uint8_t data) {
    nrfx_err_t res;
    const nrfx_twi_xfer_desc_t desc = NRFX_TWI_XFER_DESC_TXTX(address, (uint8_t*)&reg, 1, (uint8_t*)&data, 1);

    // Send the data
    res = nrfx_twi_xfer(&instance, &desc, NRFX_TWI_FLAG_TX_NO_STOP);
    APP_ERROR_CHECK(res);
    wait_for_transfer();

    // Read the register to verify correctness
    NRFX_ASSERT(data == read_byte(reg));
}

static inline void apply_settings() {
    uint8_t reg = read_byte(USR_REG);

    // Set the resolution to 00 (max)
    reg &= ~((1<<USR_RES0) | (1<<USR_RES1));

    write_byte(USR_REG, reg);
}

void temperature_sensor_init() {
    nrfx_err_t res;

    // Set the configuration
    const nrfx_twi_config_t config = {                                                \
        .frequency          = NRF_TWI_FREQ_400K,                                      \
        .scl                = 12,                                                     \
        .sda                = 11,                                                     \
        .interrupt_priority = NRFX_TWI_DEFAULT_CONFIG_IRQ_PRIORITY,                   \
        .hold_bus_uninit    = NRFX_TWI_DEFAULT_CONFIG_HOLD_BUS_UNINIT,                \
    };

    res = nrfx_twi_init(&instance, &config, twi_handler, NULL);
    APP_ERROR_CHECK(res);

    nrfx_twi_enable(&instance);

    reset();

    apply_settings();
}

void temperature_sensor_read() {
    // nrfx_err_t res;
    // uint8_t RX[1] = {0};
    // const nrfx_twi_xfer_desc_t desc = NRFX_TWI_XFER_DESC_TXRX(address, (uint8_t*)USR_REG, sizeof(USR_REG), RX, sizeof(RX));

    // res = nrfx_twi_xfer(&instance, &desc, NRFX_TWI_FLAG_TX_NO_STOP);
    // APP_ERROR_CHECK(res);

    // wait_for_transfer();

    // NRF_LOG_INFO("read RX0: %i", RX[0]);
    NRF_LOG_INFO("read RX0: %i", read_byte(USR_REG));
}
