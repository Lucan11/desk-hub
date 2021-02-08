#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include <stdint.h>


// Forward declaration of struct, to prevent a include
typedef struct _temperature_sensor_data temperature_sensor_data_t;
typedef struct _pixel pixel_t;


void display_init();


void display_set_sensor_data(const temperature_sensor_data_t * const inside_data, const temperature_sensor_data_t * const outside_data);


// Can only draw straight lines
// So xs == xe and ys != ye
// Or ys == ye and xs != xe
void display_draw_line(const uint8_t xs, const uint8_t ys, const uint8_t xe, const uint8_t ye, const pixel_t * const color);




#endif//_DISPLAY_H_
