#include "display.h"

#include <string.h>

#include "ST7735.h"


void display_init(){
    ST7735_init();
}

void display_set_sensor_data(const temperature_sensor_data_t * const data) {
    ST7735_draw_character(2, 2, 'c');
}
