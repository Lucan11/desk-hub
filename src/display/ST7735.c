#include "ST7735.h"

#include "nrf_gpio.h"
#include "nrfx_spi.h"
#include "nrf_delay.h"

#include "log.h"
#include "font.h"


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


// The display size is not the max that the ST7735 can support (132x162)
// The reported display size is 128x160, but I'm not sure if I missed a
// configuration in a register. But I need these offsets to be able to
// write from begin to end, otherwise we write outside of the display bounds
#define DISPLAY_X_START_OFFSET  ((uint8_t)2)
#define DISPLAY_X_STOP_OFFSET   ((uint8_t)129)
#define DISPLAY_Y_START_OFFSET  ((uint8_t)1)
#define DISPLAY_Y_STOP_OFFSET   ((uint8_t)160)
#define DISPLAY_WIDTH           (DISPLAY_X_STOP_OFFSET - DISPLAY_X_START_OFFSET)
#define DISPLAY_HIGHT           (DISPLAY_Y_STOP_OFFSET - DISPLAY_Y_START_OFFSET)
#define DISPLAY_NUM_PIXELS      ((DISPLAY_WIDTH + 1) * (DISPLAY_HIGHT + 1))


#define PIXEL_RED_BITS          5
#define PIXEL_GREEN_BITS        6
#define PIXEL_BLUE_BITS         5
#define PIXEL_RED_MAX_VALUE     31
#define PIXEL_GREEN_MAX_VALUE   63
#define PIXEL_BLUE_MAX_VALUE    31


#define SWRESET 0x01
#define RDDID   0x04
#define RDDST   0x09
#define SLPIN   0x10
#define SLPOUT  0x11
#define PTLON   0x12
#define NORON   0x13
#define INVOFF  0x20
#define INVON   0x21
#define DISPOFF 0x28
#define DISPON  0x29
#define RAMRD   0x2E
#define CASET   0x2A
#define RASET   0x2B
#define RAMWR   0x2C
#define PTLAR   0x30
#define MADCTL  0x36
#define COLMOD  0x3A
#define FRMCTR1 0xB1
#define FRMCTR2 0xB2
#define FRMCTR3 0xB3
#define INVCTR  0xB4
#define DISSET5 0xB6
#define PWCTR1  0xC0
#define PWCTR2  0xC1
#define PWCTR3  0xC2
#define PWCTR4  0xC3
#define PWCTR5  0xC4
#define VMCTR1  0xC5
#define RDID1   0xDA
#define RDID2   0xDB
#define RDID3   0xDC
#define RDID4   0xDD
#define GMCTRP1 0xE0
#define GMCTRN1 0xE1
#define PWCTR6  0xFC


// SPI and I2C use the same hardware, so use SPI1 instead of 0
static nrfx_spi_t instance = NRFX_SPI_INSTANCE(1);



struct _current_bounds {
    uint8_t xs;
    uint8_t xe;
    uint8_t ys;
    uint8_t ye;
    uint8_t xdim;
    uint8_t ydim;
    uint16_t num_pixels;
} static current_bounds;


static void ST7735_gpio_init();
static void ST7735_spi_init();
static void ST7735_spi_event_handler(nrfx_spi_evt_t const * p_event, void * p_context);
static void ST7735_reset();
static void display_configure();
static inline void pixel_set_color(pixel_t * const pixel, pixel_colors_t color, const uint8_t value);
static inline void wait_for_transfer();
static inline void ST7735_send_command(uint8_t command);
static inline void ST7735_send_data(uint8_t data);
static inline void transfer_pixel(const pixel_t * const pixel);


static volatile unsigned int transfer_done = 0;
static inline void wait_for_transfer() {
    // NRF_LOG_INFO("waiting for transfer!");
    while (transfer_done == 0) {
        // maybe sleep here
    }

    transfer_done = 0;
}


// Need to send the RAMRW command before this function!
static inline void transfer_pixel(const pixel_t * const pixel) {
    nrfx_err_t err;
    const nrfx_spi_xfer_desc_t xfer = NRFX_SPI_XFER_TX(&pixel->raw_data, 2);

    err = nrfx_spi_xfer(&instance, &xfer, 0);
    APP_ERROR_CHECK(err);

    wait_for_transfer();
}



void ST7735_draw_character( const uint8_t x,
                            const uint8_t y,
                            const char character) {
    pixel_t pixel = {0};

    ST7735_set_draw_area(x, x + DISPLAY_CHAR_WIDTH, y, y + DISPLAY_CHAR_WIDTH);

    ST7735_set_color(&pixel);

    // Memory write command
    ST7735_send_command(RAMWR);

    for (size_t i = 0; i < DISPLAY_CHAR_WIDTH; i++) {
        for (size_t j = 0; j < DISPLAY_CHAR_WIDTH; j++) {
            if((FONTS[character - 32][i] >> j) & 0x1)
                pixel_set_color(&pixel, red, PIXEL_RED_MAX_VALUE);
            else
                pixel.raw_data = 0xffff;

            transfer_pixel(&pixel);
        }
    }
}



void ST7735_set_draw_area(  const uint8_t xs,
                            const uint8_t xe,
                            const uint8_t ys,
                            const uint8_t ye)
{
    // Ensure that we are within bounds
    NRFX_ASSERT(xs >= DISPLAY_X_START_OFFSET);
    NRFX_ASSERT(xe <= DISPLAY_X_STOP_OFFSET);
    NRFX_ASSERT(ys >= DISPLAY_Y_START_OFFSET);
    NRFX_ASSERT(ye <= DISPLAY_Y_STOP_OFFSET);

    // Update the bounds parameters
    current_bounds.xs = xs;
    current_bounds.xe = xe;
    current_bounds.ys = ys;
    current_bounds.ye = ye;
    current_bounds.xdim = current_bounds.xe - current_bounds.xs + 1;
    current_bounds.ydim = current_bounds.ye - current_bounds.ys + 1;
    current_bounds.num_pixels = current_bounds.xdim * current_bounds.ydim;

    // Set the column (xs - xe)
    ST7735_send_command(CASET);
    ST7735_send_data(0x00);
    ST7735_send_data(xs);
    ST7735_send_data(0x00);
    ST7735_send_data(xe);

    // Set the row (ys - ye)
    ST7735_send_command(RASET);
    ST7735_send_data(0x00);
    ST7735_send_data(ys);
    ST7735_send_data(0x00);
    ST7735_send_data(ye);
}

static inline void ST7735_send_command(uint8_t command) {
    nrfx_err_t err;
    const nrfx_spi_xfer_desc_t xfer = NRFX_SPI_XFER_TX(&command, 1);

    nrf_gpio_pin_clear(DATA_COMMAND_PIN);

    err = nrfx_spi_xfer(&instance, &xfer, 0);
    APP_ERROR_CHECK(err);

    wait_for_transfer();

    nrf_gpio_pin_set(DATA_COMMAND_PIN);
}


static inline void ST7735_send_data(uint8_t data) {
    nrfx_err_t err;
    const nrfx_spi_xfer_desc_t xfer = NRFX_SPI_XFER_TX(&data, 1);

    // Make sure it is set
    nrf_gpio_pin_set(DATA_COMMAND_PIN);

    err = nrfx_spi_xfer(&instance, &xfer, 0);
    APP_ERROR_CHECK(err);

    wait_for_transfer();
}


void ST7735_set_color(const pixel_t * const color) {

    // Memory write command
    ST7735_send_command(RAMWR);

    for(uint16_t i = 0; i < current_bounds.num_pixels; i++){
        transfer_pixel(color);
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


static void ST7735_spi_event_handler(nrfx_spi_evt_t const * p_event, void * p_context) {
    // nrfx_spi_1_irq_handler();
    transfer_done = 1;
}


static void ST7735_reset(){
    // First hw reset
    nrf_gpio_pin_clear(RESET_PIN);

    nrf_delay_us(RESET_DOWN_TIME_US);

    nrf_gpio_pin_set(RESET_PIN);

    nrf_delay_ms(POWER_UP_TIME_MS); // Not sure if this one also needs to be this long

    // software reset
    ST7735_send_command(SWRESET);

    nrf_delay_ms(POWER_UP_TIME_MS);
}


static void ST7735_gpio_init() {
    // Configure the gpio pins as output
    nrf_gpio_cfg_output(RESET_PIN);
    nrf_gpio_cfg_output(DATA_COMMAND_PIN);
    nrf_gpio_cfg_output(BACK_LIGHT_PIN);

    // Set the correct output
    nrf_gpio_pin_set(RESET_PIN);
    nrf_gpio_pin_set(DATA_COMMAND_PIN);
    nrf_gpio_pin_set(BACK_LIGHT_PIN);
}


static void ST7735_spi_init() {
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

static void display_configure() {
    // Go out of sleep mode
    ST7735_send_command(SLPOUT);

    // 16 bit per pixel command (RGB-5-6-5)
    ST7735_send_command(COLMOD);
    ST7735_send_data(0b00000101);

    // Set the correct bounds, such that we dont write out of bounds
    ST7735_set_draw_area(   DISPLAY_X_START_OFFSET,
                            DISPLAY_X_STOP_OFFSET,
                            DISPLAY_Y_START_OFFSET,
                            DISPLAY_Y_STOP_OFFSET);

    // Turn the screen on
    ST7735_send_command(DISPON);
}

void ST7735_init() {
    ST7735_gpio_init();

    ST7735_spi_init();

    ST7735_reset();

    display_configure();
}
