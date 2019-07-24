#ifndef __SPI_H__
#define __SPI_H__

#include "lpc_types.h"

void SD_SPI_Init(void);
uint8_t SD_SPI_WriteAndRead(uint8_t data);

#endif
