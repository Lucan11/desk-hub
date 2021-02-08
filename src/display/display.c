#include "display.h"

#include <stdlib.h>
#include <string.h>

#include "ST7735.h"
#include "Si7021.h"


#define INSIDE_STR      "-Inside-"
#define OUTSIDE_STR     "-Outside-"
#define TEMP_STR        "temp: "
#define HUMI_STR        "humi: "


void display_init(){
    ST7735_init();

    ST7735_draw_string(5, 5, INSIDE_STR, strlen(INSIDE_STR));
    ST7735_draw_string(5, 160 - (8*strlen(OUTSIDE_STR)) - 5, OUTSIDE_STR, strlen(OUTSIDE_STR));

    ST7735_draw_string(15, 5, TEMP_STR, strlen(TEMP_STR));
    ST7735_draw_string(25, 5, HUMI_STR, strlen(HUMI_STR));
}

void display_set_sensor_data(   const temperature_sensor_data_t * const inside_data,
                                const temperature_sensor_data_t * const outside_data) {
    char buffer[5];

    // inside data
    __itoa(inside_data->temperature, buffer, 10);
    ST7735_draw_string(15, 5 + strlen(TEMP_STR)*8, buffer, strlen(buffer));
    __itoa(inside_data->humidity, buffer, 10);
    ST7735_draw_string(25, 5 + strlen(HUMI_STR)*8, buffer, strlen(buffer));

    // Outside data
    __itoa(outside_data->temperature, buffer, 10);
    ST7735_draw_string(15, 110, buffer, strlen(buffer));
    __itoa(outside_data->humidity, buffer, 10);
    ST7735_draw_string(25, 110, buffer, strlen(buffer));
}
