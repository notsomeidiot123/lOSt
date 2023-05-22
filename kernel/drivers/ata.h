#pragma once
#include "storage.h"

extern int ata_identify_all();
extern void ata28_read(uint16_t *buffer, ata_drive32_t *drive, int sectors, int start);