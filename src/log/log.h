#ifndef _LOG_H_
#define _LOG_H_

#include "nrf_log.h"

void log_init();

// If deferred loggin is enabled, call this function to actually log the data
uint8_t log_flush();


#endif//_LOG_H_
