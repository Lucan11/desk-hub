#include "spi.h"

#include "nrfx_spi.h"



// SPI and I2C use the same hardware, so use SPI1 instead of 0
static nrfx_spi_t instance = NRFX_SPI_INSTANCE(1);


// These names are I2C, but the interface is actualy SPI
// Thanks, aliexpress
#define SCL_PIN             18
#define SDA_PIN             19
#define CHIP_SELECT_PIN     23
#define MOSI_PIN            SDA_PIN
#define SCK_PIN             SCL_PIN



static volatile unsigned int transfer_done = 0;


static void ST7735_spi_event_handler(nrfx_spi_evt_t const * p_event, void * p_context) {
    transfer_done = 1;
}


static inline void wait_for_transfer() {
    while (transfer_done == 0) {
        // maybe sleep here
    }

    transfer_done = 0;
}



void spi_transfer(const uint8_t * const data, const size_t data_len) {
    nrfx_err_t err;
    const nrfx_spi_xfer_desc_t xfer = NRFX_SPI_XFER_TX(data, data_len);

    NRFX_ASSERT(data != NULL);
    NRFX_ASSERT(data_len > 0);

    err = nrfx_spi_xfer(&instance, &xfer, 0);
    APP_ERROR_CHECK(err);

    // Maybe move this to before the function? Overlap execution.
    // Does not work out-of-the-box for commands, need to toggle the command pin
    // Maybe add a 'bool blocking' parameter to set this?
    wait_for_transfer();
}


void ST7735_spi_init() {
    nrfx_err_t err;

    const nrfx_spi_config_t config  = {
        .sck_pin = SCK_PIN,
        .mosi_pin = MOSI_PIN,
        .miso_pin = NRFX_SPI_PIN_NOT_USED,
        .ss_pin = CHIP_SELECT_PIN,
        .irq_priority = NRFX_SPI_DEFAULT_CONFIG_IRQ_PRIORITY,
        .orc = 0xFF, // what to transmit when receiving. Not sure if it matters in our case
        .frequency = NRF_SPI_FREQ_8M,
        .mode = NRF_SPI_MODE_0,
        .bit_order = NRF_SPI_BIT_ORDER_MSB_FIRST
    };

    err = nrfx_spi_init(&instance,
                        &config,
                        ST7735_spi_event_handler,
                        NULL);
    APP_ERROR_CHECK(err);
}
