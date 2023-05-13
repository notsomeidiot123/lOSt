#include "storage.h"
#include "../cpu/ecodes.h"
#include "ata.h"
#include "floppy.h"
#include <stdint.h>


drive_t drives[16] = { 0 };

int register_drive(drive_t drive_to_register){
    for(int i = 0; i < 16; i++){
        if(drives[i].type == 0){
            drives[i] = drive_to_register;
            return 0;
        }
    }
    return -1;
}

int read_from_drive(uint16_t *buffer, int sectors, int start, int drive){
    switch(drives[drive].type){
        case DRIVE_NULL:
            return DE_INVALID_DRIVE;
        case DRIVE_PATA:
            // return ata_read()
            break;
        default:
            return E_NOT_IMPLEMENTED;
    }
    return E_NO_ERR;
}