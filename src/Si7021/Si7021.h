#ifndef _SI7021_H_
#define _SI7021_H_

typedef struct _temperature_sensor_data {
    float temperature;
    float humidity;
} temperature_sensor_data_t;


void temperature_sensor_init();

temperature_sensor_data_t temperature_sensor_read();




#endif//_SI7021_H_
