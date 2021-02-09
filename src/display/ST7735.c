#include "ST7735.h"

#include <string.h>

#include "nrf_gpio.h"
#include "nrf_delay.h"

#include "log.h"
#include "font.h"
#include "spi.h"


#define RESET_PIN           20          // 1 is function, 0 is reset
#define DATA_COMMAND_PIN    22          // 1 is data, 0 is command
#define BACK_LIGHT_PIN      24          // (PWM) higher is more backlight

#define POWER_UP_TIME_MS    120         // Time in ms that the device needs to power up after reset
#define RESET_DOWN_TIME_US  10          // Time in us that the reset pin needs to be down for the reset to trigger


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



// Data structure to keep track of the current screen bounds
// Used by functions that draw to the screen
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
static void ST7735_reset();
static void display_configure();
static inline void ST7735_send_command(uint8_t command);
static inline void ST7735_send_data(uint8_t data);
static inline void transfer_pixel(const pixel_t * const pixel);




// Need to send the RAMRW command before this function!
static inline void transfer_pixel(const pixel_t * const pixel) {
    NRFX_ASSERT(pixel != NULL);

    spi_transfer((uint8_t*)&pixel->raw_data, 2);
}

void ST7735_draw_string(const uint8_t x, const uint8_t y, const char * const string, const pixel_t * const color, const size_t scale) {
    NRFX_ASSERT(string != NULL);
    NRFX_ASSERT(scale > 0);

    const size_t len = strlen(string);

    // for every character in the string
    for (size_t i = 0; i < len; i++) {
        ST7735_draw_character(x, y + (i*FONT_NUM_ROWS*scale), string[i], color, scale);
    }
}


void ST7735_draw_character( const uint8_t x,
                            const uint8_t y,
                            const char character,
                            const pixel_t * const color,
                            const size_t scale) {
    NRFX_ASSERT(character >= 0);
    NRFX_ASSERT(character < FONT_MAX_CHAR);
    NRFX_ASSERT(scale > 0);

    const pixel_t pixel = { .raw_data = 0xffff };
    const size_t x_len = FONT_NUM_ROWS * scale;
    const size_t y_len = FONT_NUM_ROWS * scale;

    pixel_t buffer[x_len][y_len];

    // Set the bounds
    ST7735_set_bounds(x, y, x_len, y_len);

    // Memory write command
    ST7735_send_command(RAMWR);

    // TODO: buffer this and write it all at once

    // Iterate over every pixel in the character
    for (size_t x = 0; x < FONT_NUM_ROWS; x++) {
        for (size_t y = 0; y < FONT_NUM_ROWS; y++) {
            // Set the color if we need to draw anything,
            // Otherwise, make the pixel white (0xffff)
            // Also, fuck using the cache, right?
            if((font8x8_basic[(uint8_t)character][y] >> x) & 0x1)
                buffer[x * scale][y * scale] = *color;
            else
                buffer[x * scale][y * scale] = pixel;
        }
    }

    // If we have a scale, than we need to fill in the empty pixels
    if(scale > 1){
        for (size_t x = 0; x < x_len; x += scale) {
            for (size_t y = 0; y < y_len; y += scale) {
                for(size_t row = 0; row < scale; row++) {
                    for(size_t col = 0; col < scale; col++) {
                        buffer[x+row][y+col] = buffer[x][y];
                    }
                }
            }
        }
    }

    spi_transfer((uint8_t*)&buffer, sizeof(buffer));
}

// Function to set the drawing bounds on the display
// valid ranges:
//      0 <= x < DISPLAY_WIDTH
//      0 <= y < DISPLAY_HEIGHT
//      0 < x_len <= DISPLAY_WIDTH
//      0 < y_len <= DISPLAY_HEIGHT
//      0 <= x + x_len < DISPLAY_WIDTH);
//      0 <= y + y_len < DISPLAY_HEIGHT);
void ST7735_set_bounds(const uint8_t x, const uint8_t y, const uint8_t x_len, const uint8_t y_len) {
    // NRF_LOG_INFO("x: %u, y: %u, x_len: %u, y_len: %u, max_x: %u, max_y: %u", x, y, x_len, y_len, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    NRFX_ASSERT(x + x_len <= DISPLAY_WIDTH);
    NRFX_ASSERT(y + y_len <= DISPLAY_HEIGHT);
    NRFX_ASSERT(x_len > 0);
    NRFX_ASSERT(y_len > 0);

    // Update the bounds parameters
    // Adjust for the offset to the display
    current_bounds.xs = x + DISPLAY_X_START_OFFSET;
    current_bounds.ys = y + DISPLAY_Y_START_OFFSET;

    // Length is not inclusive, so remove one
    current_bounds.xe = current_bounds.xs + x_len - 1;
    current_bounds.ye = current_bounds.ys + y_len - 1;

    current_bounds.xdim = current_bounds.xe - current_bounds.xs + 1;
    current_bounds.ydim = current_bounds.ye - current_bounds.ys + 1;
    current_bounds.num_pixels = (current_bounds.xdim) * (current_bounds.ydim);

    // NRF_LOG_INFO("xs: %u, ys: %u, xe: %u, ye: %u", current_bounds.xs, current_bounds.ys, current_bounds.xe, current_bounds.ye);

    // Set the column (xs - xe)
    ST7735_send_command(CASET);
    ST7735_send_data(0x00);
    ST7735_send_data(current_bounds.xs);
    ST7735_send_data(0x00);
    ST7735_send_data(current_bounds.xe);

    // Set the row (ys - ye)
    ST7735_send_command(RASET);
    ST7735_send_data(0x00);
    ST7735_send_data(current_bounds.ys);
    ST7735_send_data(0x00);
    ST7735_send_data(current_bounds.ye);
}

static inline void ST7735_send_command(uint8_t command) {

    nrf_gpio_pin_clear(DATA_COMMAND_PIN);

    spi_transfer(&command, 1);

    nrf_gpio_pin_set(DATA_COMMAND_PIN);
}


static inline void ST7735_send_data(uint8_t data) {
    // Make sure it is set
    nrf_gpio_pin_set(DATA_COMMAND_PIN);

    spi_transfer(&data, 1);
}


void ST7735_fill_bounds(const pixel_t * const color) {
    NRFX_ASSERT(color != NULL);

    // Memory write command
    ST7735_send_command(RAMWR);

    // We can speed this up if we use a small buffer to send more data at once
    // skips initialization overhead of SPI transfer
    for(uint16_t i = 0; i < current_bounds.num_pixels; i++){
        transfer_pixel(color);
    }
}

// TODO: set value to a percentage (0 - COLOR_MAX)
void pixel_set_color(pixel_t * const pixel, const pixel_colors_t color, const uint8_t percentage) {
    NRFX_ASSERT(pixel != NULL);
    NRFX_ASSERT(percentage <= 100);

    switch (color) {
    case red:
        pixel->colors.red = (PIXEL_RED_MAX_VALUE * percentage)/100;
        break;

    case green:;
        uint8_t value = (PIXEL_GREEN_MAX_VALUE*percentage)/100;
        pixel->colors.green_high = (value & 0b111000) >> 3;
        pixel->colors.green_low  = (value & 0b000111);
        break;

    case blue:
        pixel->colors.blue = (PIXEL_BLUE_MAX_VALUE * percentage)/100;
        break;

    default:
        NRF_LOG_INFO("display incorrect color value: %i", color);
        NRFX_ASSERT(0);
        break;
    }
}


static void ST7735_reset(){
    // We need to do both a HW and SF reset since they reset different things
    // Nether of them clears the RAM, be mindfull of this.

    // First hw reset
    nrf_gpio_pin_clear(RESET_PIN);

    nrf_delay_us(RESET_DOWN_TIME_US);

    nrf_gpio_pin_set(RESET_PIN);

    nrf_delay_ms(5);

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


static void display_configure() {
    // Go out of sleep mode
    ST7735_send_command(SLPOUT);

    // 16 bit per pixel command (RGB-5-6-5)
    ST7735_send_command(COLMOD);
    ST7735_send_data(0b00000101);

    // Invert x-axis (MV-X) to orient the display correctly
    ST7735_send_command(MADCTL);
    ST7735_send_data(0b01000000);

    // Set the correct bounds, such that we dont write out of bounds
    ST7735_set_bounds(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    // Set the display color to white
    pixel_t color = {.raw_data = 0xffff};
    ST7735_fill_bounds(&color);

    // Turn the screen on
    ST7735_send_command(DISPON);
}

void ST7735_init() {
    ST7735_gpio_init();

    ST7735_spi_init();

    ST7735_reset();

    display_configure();
}
