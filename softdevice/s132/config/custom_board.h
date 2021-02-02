#ifndef _CUSTOM_BOARD_H_
#define _CUSTOM_BOARD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "nrf_gpio.h"

#define LEDS_NUMBER    1

#define LED_START      7
#define LED_1          7
#define LED_STOP       7

#define LEDS_ACTIVE_STATE 0

#define LEDS_LIST { LED_1 }

// Unused
#define BUTTONS_NUMBER 0
#define BUTTONS_ACTIVE_STATE 0

#ifdef __cplusplus
}
#endif


#endif//_CUSTOM_BOARD_H_
