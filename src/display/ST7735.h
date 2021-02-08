#ifndef _ST7735_H_
#define _ST7735_H_

#include <stdint.h>

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
} pixel_t;

typedef enum _pixel_colors {
    red,
    green,
    blue
} pixel_colors_t;


void ST7735_draw_character(const uint8_t x, const uint8_t y, const char character);

void ST7735_set_color(const pixel_t * const color);


void ST7735_init();

#endif//_ST7735_H_
