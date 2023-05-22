#include "storage.h"
#include "../cpu/ecodes.h"
#include "ata.h"
#include "floppy.h"
#include <stdint.h>


drive32_t *filesystems[26] = { 0 };
drive32_t *drives[26] = { 0 };

int register_drive(drive32_t *drive_to_register){
    for(int i = 0; i < 26; i++){
        if(drives[i] == 0){
            drives[i] = drive_to_register;
            drives[i]->number = i;
            return 0;
        }
    }
    return -1;
}

int read_file(uint16_t *buffer, char *filename){
    return 0;   
}

int read_from_drive(uint16_t *buffer, int sectors, int start, int drive){
    switch(drives[drive]->type){
        case DRIVE_NULL:
            return DE_INVALID_DRIVE;
        case DRIVE_PATA28:
            // return ata_read()
            break;
        default:
            return E_NOT_IMPLEMENTED;
    }
    return E_NO_ERR;
}
int get_drive_count(){
    int r = 0;
    for(int i = 0; i < 26; i++){
        if(drives[i] != 0) r++;
    }
    return r;
}