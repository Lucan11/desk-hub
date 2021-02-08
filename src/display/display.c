#include "display.h"

#include <stdlib.h>
#include <string.h>

#include "nrf_assert.h"

#include "ST7735.h"
#include "Si7021.h"
#include "font.h"

#define ROW             (FONT_NUM_ROWS + 1)
#define SCALE           3

#define INSIDE_STR      "In"
#define OUTSIDE_STR     "Out"
#define TEMP_STR        "temp: "
#define HUMI_STR        "humi: "


void display_init(){
    pixel_t color = { 0 };

    ST7735_init();

    pixel_set_color(&color, red, 10);
    pixel_set_color(&color, green, 10);
    pixel_set_color(&color, blue, 31);

    // Draw inside and outside at the top of the display
    ST7735_draw_string(ROW/2, FONT_NUM_ROWS, INSIDE_STR, &color, SCALE);
    ST7735_draw_string(ROW/2, DISPLAY_HEIGHT - (FONT_NUM_ROWS*strlen(OUTSIDE_STR)*SCALE), OUTSIDE_STR, &color, SCALE);

    // Draw the temp and humi strings below this
    ST7735_draw_string(ROW*2, 5, TEMP_STR, &color, SCALE);
    ST7735_draw_string(ROW*3, 5, HUMI_STR, &color, SCALE);


    display_draw_line(13, DISPLAY_HEIGHT/2, DISPLAY_WIDTH - 10, DISPLAY_HEIGHT/2, &color);
    display_draw_line(13, 10, 13, DISPLAY_HEIGHT - 10, &color);
}


void display_set_sensor_data(   const temperature_sensor_data_t * const inside_data,
                                const temperature_sensor_data_t * const outside_data) {
    char buffer[5];
    const pixel_t color = {
        .colors.red = 10,
        .colors.blue = 31,
        .colors.green_high = (10 & 0b111000) >> 3,
        .colors.green_low = 10 & 0b000111
    };

    // This offset is the size of the TEMP/HUMI string
    uint8_t y_offset = FONT_NUM_ROWS + strlen(TEMP_STR) * 8 * SCALE;

    // inside data
    __itoa(inside_data->temperature, buffer, 10);
    ST7735_draw_string(ROW*2, y_offset, buffer, &color, SCALE);
    __itoa(inside_data->humidity, buffer, 10);
    ST7735_draw_string(ROW*3, y_offset, buffer, &color, SCALE);

    y_offset += FONT_NUM_ROWS * 6;

    // Outside data
    __itoa(outside_data->temperature, buffer, 10);
    ST7735_draw_string(ROW*2, y_offset, buffer, &color, SCALE);
    __itoa(outside_data->humidity, buffer, 10);
    ST7735_draw_string(ROW*3, y_offset, buffer, &color, SCALE);
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
