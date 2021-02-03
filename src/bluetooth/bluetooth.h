#ifndef _BLUETOOTH_H_
#define _BLUETOOTH_H_

// Forward declaration of struct, to prevent a include
typedef struct _temperature_sensor_data temperature_sensor_data_t;

// Initialize bluetooth
void bluetooth_init();

// Function to scan the ble advertisements for the data from the sensor outside
temperature_sensor_data_t bluetooth_get_outside_temperature();


#endif//_BLUETOOTH_H_
