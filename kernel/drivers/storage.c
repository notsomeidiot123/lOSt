#include "storage.h"

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