#ifndef _DISPLAY_H_
#define _DISPLAY_H_

// Forward declaration of struct, to prevent a include
typedef struct _temperature_sensor_data temperature_sensor_data_t;

void display_init();

void display_set_sensor_data(const temperature_sensor_data_t * const data);



#endif//_DISPLAY_H_
