#ifndef _SPI_H_
#define _SPI_H_

#include <stdint.h> // uint8_t
#include <stddef.h> // size_t


void spi_transfer(const uint8_t * const data, const size_t data_len);

void ST7735_spi_init();

#endif//_SPI_H_
