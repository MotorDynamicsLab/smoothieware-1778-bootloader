#ifndef __SD_CARD_H__
#define __SD_CARD_H__

#include "lpc_types.h"

void SDCard_Init(void);
int SDCard_disk_initialize(void);
int SDCard_disk_write(const uint8_t *buffer, uint32_t block_number);
int SDCard_disk_read(uint8_t *buffer, uint32_t block_number);
int SDCard_disk_status(void);
int SDCard_disk_sync(void);
uint32_t SDCard_disk_sectors(void);
uint64_t SDCard_disk_size(void);
uint32_t SDCard_disk_blocksize(void);
int SDCard_disk_erase(uint32_t block_number, int count);
int SDCard_disk_canDMA(void);
void dma_source_event(void);
void dma_dest_event(void);

int busy(void);

#endif
