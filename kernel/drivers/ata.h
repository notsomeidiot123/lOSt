#pragma once
#include "storage.h"
#include <stdint.h>

extern int ata_identify_all();
extern uint16_t *ata28_read(uint16_t *buffer, ata_drive32_t *drive, uint32_t sectors, uint32_t start);
extern uint16_t *ata_read(uint16_t *buffer, ata_drive32_t *drive, uint32_t sectors, uint32_t start);
extern uint16_t ata28_write(uint16_t *buffer, ata_drive32_t *drive, uint32_t sectors, uint32_t start);
extern uint16_t ata_write(uint16_t *buffer, ata_drive32_t *drive, uint32_t sectors, uint32_t start);