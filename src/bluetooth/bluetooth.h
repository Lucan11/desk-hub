#ifndef _BLUETOOTH_H_
#define _BLUETOOTH_H_

// Forward declaration of struct, to prevent a include
typedef struct _temperature_sensor_data temperature_sensor_data_t;

// Initialize bluetooth
void bluetooth_init();


// Start the bluetooth advertisements
void bluetooth_start_advertisement();


// Update the bluetooth advertising data, data will be truncated if too big
void bluetooth_update_advertisement_data(const temperature_sensor_data_t * const data);


#endif//_BLUETOOTH_H_
