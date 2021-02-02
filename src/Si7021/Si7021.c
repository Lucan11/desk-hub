#include "Si7021.h"

#include "app_error.h"
#include "nrfx_twi.h"
#include "nrf_delay.h"
#include <nrf_assert.h>
#include "log.h"

// Device specific commands
static const uint8_t power_up_time_ms = 15;
static const uint8_t address = 0x40;
static const uint8_t RST_CMD = 0xFE;
static const uint8_t MSR_TMP_CMD = 0xE3;
static const uint8_t USR_REG = 0xE7;
static const uint8_t USR_RES0 = 0;
static const uint8_t USR_RES1 = 7;

static const nrfx_twi_t instance = NRFX_TWI_INSTANCE(0);    // Create an TWI instance using register 0
static volatile unsigned int transfer_done = 0;

static inline void wait_for_transfer();
static void twi_handler(nrfx_twi_evt_t const * p_event, void * p_context);
static inline void read_bytes(const uint8_t * const tx, const size_t tx_len, uint8_t * rx, const size_t rx_len);
static inline uint8_t read_register(const uint8_t reg);
static inline void reset();
static inline void write_byte(const uint8_t reg, const uint8_t data);
static inline void apply_settings();
static inline float tmp_code_to_float(const uint16_t code);


static inline void wait_for_transfer() {
    while(transfer_done == 0) {
        // Maybe go to sleep here?
    }

    transfer_done = 0;
}

static void twi_handler(nrfx_twi_evt_t const * p_event, void * p_context) {
    nrfx_twi_0_irq_handler();
    transfer_done = 1;
}

static inline void read_bytes(const uint8_t * const tx, const size_t tx_len, uint8_t * rx, const size_t rx_len) {
    nrfx_err_t res;
    const nrfx_twi_xfer_desc_t desc = NRFX_TWI_XFER_DESC_TXRX(address, (uint8_t*)tx, tx_len, rx, rx_len);

    // Clear the rx array
    memset(rx, 0x00, rx_len);

    // Send the read command
    res = nrfx_twi_xfer(&instance, &desc, 0);
    APP_ERROR_CHECK(res);
    wait_for_transfer();
}

static inline uint8_t read_register(const uint8_t reg) {
    uint8_t value;
    read_bytes(&reg, 1, &value, 1);
    return value;
}

static inline void reset() {
    // Send the reset command and ignore the return value
    (void)read_register(RST_CMD);

    nrf_delay_ms(power_up_time_ms);
}

static inline void write_byte(const uint8_t reg, const uint8_t data) {
    nrfx_err_t res;
    uint8_t value;
    const nrfx_twi_xfer_desc_t desc = NRFX_TWI_XFER_DESC_TXTX(address, (uint8_t*)&reg, 1, (uint8_t*)&data, 1);

    // Write to the register
    res = nrfx_twi_xfer(&instance, &desc, 0);
    APP_ERROR_CHECK(res);
    wait_for_transfer();

    // Read the register to verify correctness
    value = read_register(reg);
    NRFX_ASSERT(data == value);
}

static inline void apply_settings() {
    uint8_t reg;

    // Read the current register value
    reg = read_register(USR_REG);

    // Set the resolution to 00 (max)
    reg &= ~((1<<USR_RES0) | (1<<USR_RES1));

    // Write the new register value
    write_byte(USR_REG, reg);
}

void temperature_sensor_init() {
    nrfx_err_t res;

    // Set the configuration to 400KHz, default config
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

static inline float tmp_code_to_float(const uint16_t code) {
    // from datasheet
    return ((175.72 * code)/65536) - 46.85;
}

void temperature_sensor_read() {
    const size_t temp_reading_num_bytes = 2;
    uint8_t rx[temp_reading_num_bytes];
    float temp;

    read_bytes(&MSR_TMP_CMD, 1, rx, temp_reading_num_bytes);

    temp = tmp_code_to_float(rx[0]<<8 | rx[1]);
    NRF_LOG_INFO("read: %i", temp*10);
}
