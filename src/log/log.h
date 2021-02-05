#ifndef _LOG_H_
#define _LOG_H_

#include "nrf_log.h"

void log_init();

// If deferred loggin is enabled, call this function to actually log the data
void log_flush();


#endif//_LOG_H_
