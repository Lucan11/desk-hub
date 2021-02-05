#include "ST7735.h"

#include "nrf_gpio.h"
#include "nrfx_spi.h"
#include "nrf_delay.h"

#include "log.h"


// These names are I2C, but the interface is actualy SPI
// Thanks, aliexpress
#define SCL_PIN             18
#define SDA_PIN             19
#define CHIP_SELECT_PIN     23
#define MOSI_PIN            SDA_PIN
#define SCK_PIN             SCL_PIN

#define RESET_PIN           20          // 1 is function, 0 is reset
#define DATA_COMMAND_PIN    22          // 1 is data, 0 is command
#define BACK_LIGHT_PIN      24          // (PWM) higher is more backlight

#define POWER_UP_TIME_MS    120         // Time in ms that the device needs to power up after reset
#define RESET_DOWN_TIME_US  10          // Time in us that the reset pin needs to be down for the reset to trigger

#define DISPLAY_WIDTH       128
#define DISPLAY_HIGHT       160
#define DISPLAY_NUM_PIXELS  ((DISPLAY_WIDTH) * (DISPLAY_HIGHT))


#define PIXEL_RED_BITS          5
#define PIXEL_GREEN_BITS        6
#define PIXEL_BLUE_BITS         5
#define PIXEL_RED_MAX_VALUE     31
#define PIXEL_GREEN_MAX_VALUE   63
#define PIXEL_BLUE_MAX_VALUE    31


// 5-6-5 RGB configuration
typedef struct _pixel {
    union
    {
        uint16_t raw_data;

        // The data needs to be send as follows:
        // Start                 --->                   end
        // D7 D6 D5 D4 D3 D2 D1 D0  D7 D6 D5 D4 D3 D2 D1 D0
        // R4 R3 R2 R1 R0 G5 G4 G3  G2 G1 G0 B4 B3 B2 B1 B0
        // Due to the endieness of the chip, this is the way we structure it.
        struct {
            uint16_t green_high : 3;
            uint16_t red : 5;
            uint16_t blue : 5;
            uint16_t green_low : 3;
        } colors;
    };
} PACKED pixel_t;


typedef enum _pixel_colors {
    red,
    green,
    blue
} pixel_colors_t;

// SPI and I2C use the same hardware, so use SPI1 instead of 0
static nrfx_spi_t instance = NRFX_SPI_INSTANCE(1);

//~40KB way too big to buffer in RAM
// static pixel_t display_buffer[DISPLAY_NUM_PIXELS] = {0};


static void display_gpio_init();
static void display_spi_init();
static void display_spi_event_handler(nrfx_spi_evt_t const * p_event, void * p_context);
static void display_reset();
static inline void pixel_set_color(pixel_t * const pixel, pixel_colors_t color, const uint8_t value);
static inline void send_framebuffer();
static inline void wait_for_transfer();
static inline void send_command(uint8_t command);
static inline void send_data(uint8_t data);

static volatile unsigned int transfer_done = 0;
static inline void wait_for_transfer() {
    // NRF_LOG_INFO("waiting for transfer!");
    while (transfer_done == 0) {
        // maybe sleep here
        log_flush();
    }

    transfer_done = 0;
}


static inline void send_command(uint8_t command) {
    nrfx_err_t err;
    const nrfx_spi_xfer_desc_t xfer = NRFX_SPI_XFER_TX(&command, 1);

    nrf_gpio_pin_clear(DATA_COMMAND_PIN);

    err = nrfx_spi_xfer(&instance, &xfer, 0);
    APP_ERROR_CHECK(err);

    wait_for_transfer();

    nrf_gpio_pin_set(DATA_COMMAND_PIN);
}


static inline void send_data(uint8_t data) {
    nrfx_err_t err;
    const nrfx_spi_xfer_desc_t xfer = NRFX_SPI_XFER_TX(&data, 1);

    // Make sure it is set
    nrf_gpio_pin_set(DATA_COMMAND_PIN);

    err = nrfx_spi_xfer(&instance, &xfer, 0);
    APP_ERROR_CHECK(err);

    wait_for_transfer();
}


static inline void send_framebuffer() {
    nrfx_err_t err;
    pixel_t pixel;
    const nrfx_spi_xfer_desc_t xfer = NRFX_SPI_XFER_TX(&pixel.raw_data, 2);

    memset(&pixel.raw_data, 0x00, sizeof(uint16_t));

    // Memory write command
    send_command(0x2C);

    // Set some colors
    for(uint16_t i = 0; i < DISPLAY_NUM_PIXELS; i++){
        pixel.raw_data = i;
        // pixel_set_color(&pixel, red, PIXEL_RED_MAX_VALUE);
        // pixel_set_color(&pixel, green, PIXEL_GREEN_MAX_VALUE);
        // pixel_set_color(&pixel, blue, PIXEL_BLUE_MAX_VALUE);

        // send the buffer
        err = nrfx_spi_xfer(&instance, &xfer, 0);
        APP_ERROR_CHECK(err);

        wait_for_transfer();
    }
}


static inline void pixel_set_color(pixel_t * const pixel, const pixel_colors_t color, const uint8_t value) {
    switch (color) {
    case red:
        NRFX_ASSERT(value <= PIXEL_RED_MAX_VALUE);
        pixel->colors.red = value;
        break;

    case green:
        NRFX_ASSERT(value <= PIXEL_GREEN_MAX_VALUE);
        pixel->colors.green_high = (value & 0b111000) >> 3;
        pixel->colors.green_low  = value & 0b000111;
        break;

    case blue:
        NRFX_ASSERT(value <= PIXEL_BLUE_MAX_VALUE);
        pixel->colors.blue = value;
        break;

    default:
        NRF_LOG_INFO("display incorrect color value: %i", color);
        NRFX_ASSERT(0);
        break;
    }
}


static void display_spi_event_handler(nrfx_spi_evt_t const * p_event, void * p_context) {
    // nrfx_spi_1_irq_handler();
    transfer_done = 1;
}


static void display_reset(){
    // First hw reset
    nrf_gpio_pin_clear(RESET_PIN);

    nrf_delay_us(RESET_DOWN_TIME_US);

    nrf_gpio_pin_set(RESET_PIN);

    nrf_delay_ms(POWER_UP_TIME_MS); // Not sure if this one also needs to be this long

    // software reset
    send_command(0x01);

    nrf_delay_ms(POWER_UP_TIME_MS);
}


static void display_gpio_init() {
    // Configure the gpio pins as output
    nrf_gpio_cfg_output(RESET_PIN);
    nrf_gpio_cfg_output(DATA_COMMAND_PIN);
    nrf_gpio_cfg_output(BACK_LIGHT_PIN);

    // Set the correct output
    nrf_gpio_pin_set(RESET_PIN);
    nrf_gpio_pin_set(DATA_COMMAND_PIN);
    nrf_gpio_pin_set(BACK_LIGHT_PIN);
}


static void display_spi_init() {
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
                        display_spi_event_handler,
                        NULL);
    APP_ERROR_CHECK(err);
}


void display_init() {
    pixel_t test_pixel;

    display_gpio_init();

    display_spi_init();

    display_reset();

    // test pixel colors alignmnet
    pixel_set_color(&test_pixel, red, 0b10101);
    pixel_set_color(&test_pixel, green, 0b111111);
    pixel_set_color(&test_pixel, blue, 0b10101);

    // NRFX_ASSERT(test_pixel.raw_data == ((0b10101 << 11) | (0b111111 << 5) | (0b10101)));
    NRFX_ASSERT(test_pixel.colors.red == 0b10101);
    // NRFX_ASSERT(test_pixel.colors.green == 0b111111);
    NRFX_ASSERT(test_pixel.colors.blue == 0b10101);

    // Go out of sleep mode
    send_command(0x11);

    // 16 bit per pixel command
    send_command(0x3A);
    // Set register
    send_data(0b00000101);

    // Turn the screen on
    send_command(0x29);


    send_framebuffer();
}
