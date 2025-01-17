#include "display.h"

#include <stdlib.h>
#include <string.h>

#include "nrf_assert.h"

#include "ST7735.h"
#include "Si7021.h"
#include "font.h"
#include "log.h"

#define SCALE_BIG       3
#define SCALE_NORMAL    2
#define SCALE_SMAL      1
#define ROW             ((FONT_NUM_ROWS) + 1)

#define INSIDE_STR      "In"
#define OUTSIDE_STR     "Out"
#define TEMP_STR        "T"
#define HUMI_STR        "H"

#define TEMP_X          (ROW*2*SCALE_NORMAL)
#define HUMI_X          (ROW*4*SCALE_NORMAL)
#define BORDER_PIXELS   4


static pixel_t color;


static inline void draw_temp(const uint8_t x, const uint8_t y, const float temp);
static inline void draw_humi(const uint8_t x, const uint8_t y, const float humi);



void display_init(){
    ST7735_init();

    pixel_set_color(&color, red, 10);
    pixel_set_color(&color, green, 10);
    pixel_set_color(&color, blue, 55);

    // Draw inside and outside at the top of the display
    ST7735_draw_string(ROW/2, FONT_NUM_ROWS, INSIDE_STR, &color, SCALE_BIG);
    ST7735_draw_string(ROW/2, DISPLAY_HEIGHT - (FONT_NUM_ROWS*strlen(OUTSIDE_STR)*SCALE_BIG), OUTSIDE_STR, &color, SCALE_BIG);

    // Draw the temp and humi strings below this
    ST7735_draw_string(TEMP_X, BORDER_PIXELS, TEMP_STR, &color, SCALE_NORMAL);
    ST7735_draw_string(HUMI_X, BORDER_PIXELS, HUMI_STR, &color, SCALE_NORMAL);

    // Draw 2 fancy lines in a T-Shape do differenciate between Outside and Inside
    display_draw_line(26, DISPLAY_HEIGHT/2, DISPLAY_WIDTH - 10, DISPLAY_HEIGHT/2, &color);
    display_draw_line(26, 10, 26, DISPLAY_HEIGHT - 10, &color);
}


void display_set_sensor_data(   const temperature_sensor_data_t * const inside_data,
                                const temperature_sensor_data_t * const outside_data) {

    // This offset is the size of the TEMP/HUMI string
    uint8_t y_offset = FONT_NUM_ROWS + (strlen(TEMP_STR) * FONT_NUM_ROWS * SCALE_NORMAL);

    // inside data
    draw_temp(TEMP_X - FONT_NUM_ROWS/2, y_offset, inside_data->temperature);
    draw_humi(HUMI_X - FONT_NUM_ROWS/2, y_offset, inside_data->humidity);

    // Offset to the middle of the screen, one normal sized character extra
    y_offset = DISPLAY_HEIGHT/2 + FONT_NUM_ROWS * SCALE_NORMAL;

    draw_temp(TEMP_X - FONT_NUM_ROWS/2, y_offset, outside_data->temperature);
    draw_humi(HUMI_X - FONT_NUM_ROWS/2, y_offset, outside_data->humidity);
}




static inline void draw_temp(const uint8_t x, const uint8_t y, const float temp) {
    char buffer[5];
    const int itemp = (int)temp;
    const unsigned int dec = (int)((temp - itemp) * 10);
    const size_t len = itemp >= 10 ? 2 : 1;

    // Draw the temperature by first drawing the whole number
    __itoa(itemp, buffer, 10);
    ST7735_draw_string(x, y, buffer, &color, SCALE_BIG);

    // Then the little °C at the end
    ST7735_draw_character(x, y + (len * FONT_NUM_ROWS * SCALE_BIG), FONT_DEGREE_MARK, &color, SCALE_SMAL);

    // Then the decimal
    __itoa(dec, buffer, 10);
    ST7735_draw_string(x + 1.5*ROW, y + (len * FONT_NUM_ROWS * SCALE_BIG), buffer, &color, SCALE_SMAL);
}



static inline void draw_humi(const uint8_t x, const uint8_t y, const float humi) {
    char buffer[5];
    const int ihumi = (int)humi;
    const unsigned int dec = (int)((humi - ihumi) * 10);
    const size_t len = ihumi >= 10 ? 2 : 1;

    // Draw the humidity by first drawing the whole number
    __itoa(ihumi, buffer, 10);
    ST7735_draw_string(x, y, buffer, &color, SCALE_BIG);

    // Then the little % at the end
    ST7735_draw_string(x, y + (len * FONT_NUM_ROWS * SCALE_BIG), "%", &color, SCALE_SMAL);

    // Then the decimal
    __itoa(dec, buffer, 10);
    ST7735_draw_string(x + 1.5*ROW, y + (len * FONT_NUM_ROWS * SCALE_BIG), buffer, &color, SCALE_SMAL);
}




// Can only draw straight lines one pixel wide
// So xs == xe and ys != ye
// Or ys == ye and xs != xe
void display_draw_line(const uint8_t xs, const uint8_t ys, const uint8_t xe, const uint8_t ye, const pixel_t * const color) {
    ASSERT((xs == xe && ys != ye) || (ys == ye && xs != xe));

    if (xs == xe)
        ST7735_set_bounds(xs, ys, 1, ye - ys);
    else
        ST7735_set_bounds(xs, ys, xe - xs, 1);

    ST7735_fill_bounds(color);
}
