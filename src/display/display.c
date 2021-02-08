#include "display.h"

#include <stdlib.h>
#include <string.h>

#include "ST7735.h"
#include "Si7021.h"
#include "font.h"

#define ROW             (FONT_NUM_ROWS + 1)

#define INSIDE_STR      "-Inside-"
#define OUTSIDE_STR     "-Outside-"
#define TEMP_STR        "temp: "
#define HUMI_STR        "humi: "


void display_init(){
    ST7735_init();

    // Draw inside and outside at the top of the display
    ST7735_draw_string(ROW/2, FONT_NUM_ROWS, INSIDE_STR);
    ST7735_draw_string(ROW/2, DISPLAY_HEIGHT - (FONT_NUM_ROWS*strlen(OUTSIDE_STR)) - FONT_NUM_ROWS, OUTSIDE_STR);

    // Draw the temp and humi strings below this
    ST7735_draw_string(ROW*2, 5, TEMP_STR);
    ST7735_draw_string(ROW*3, 5, HUMI_STR);
}


void display_set_sensor_data(   const temperature_sensor_data_t * const inside_data,
                                const temperature_sensor_data_t * const outside_data) {
    char buffer[5];
    uint8_t y_offset = FONT_NUM_ROWS + strlen(TEMP_STR)*8;

    // inside data
    __itoa(inside_data->temperature, buffer, 10);
    ST7735_draw_string(ROW*2, y_offset, buffer);
    __itoa(inside_data->humidity, buffer, 10);
    ST7735_draw_string(ROW*3, y_offset, buffer);

    y_offset += FONT_NUM_ROWS * 6;

    // Outside data
    __itoa(outside_data->temperature, buffer, 10);
    ST7735_draw_string(ROW*2, y_offset, buffer);
    __itoa(outside_data->humidity, buffer, 10);
    ST7735_draw_string(ROW*3, y_offset, buffer);
}
