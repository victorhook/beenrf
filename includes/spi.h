#ifndef SPI_H
#define SPI_H

#include "stdint.h"

uint8_t spi_init();

uint8_t spi_exchange(const uint8_t data);

uint8_t spi_send(const uint8_t data);

#endif /* SPI_H */
