//change read/write to be loaded as helper processes with highest priority when we switch back to multitasking
#include "storage.h"
#include "../memory/mmanager.h"
#include "../cpu/io.h"
#include "../graphics/vga.h"
#include "../cpu/idt.h"
#include "../cpu/ecodes.h"
#include "timer.h"
#include <stdint.h>

#define DATA + 0
#define ERROR + 1
#define FEATURES + 1
#define SECTOR_COUNT + 2
#define SECTOR_NUMBER + 3
#define CYLINDER_LOW + 4
#define CYLINDER_HIGH + 5
#define DRIVE_HEAD + 6
#define STATUS + 7
#define COMMAND + 7

#define LBA_LOW SECTOR_NUMBER
#define LBA_MID CYLINDER_LOW
#define LBA_HIH CYLINDER_HIGH

#define ALT_STATUS + 0
#define DEVICE_CONTROL + 0
#define DRIVE_ADDRESS + 1

#define IDENTIFY 0xEC

#define SLAVE_DRIVE 0xF0
#define MASTER_DRIVE 0xE0

#define IS_ATA(a) a[0] & 0x8000
#define GET_SZ28(a) (a[60] | (a[61] << 16))
#define IS_LBA48(a) (a[83] & (1 << 10))
#define GET_SZ48L(a) (a[100] | (a[101] << 16))
#define GET_SZ48H(a) (a[102] | (a[103] << 16))

int poll_drive_i(uint16_t port){
    wait_secs(5);
    while((inb(port STATUS) & 0x80 )){
        if(wait_secs(0)){
            return E_TIMEOUT;
        }
    }
    if(inb(port LBA_HIH) || inb(port LBA_MID)){
        return E_NOT_IMPLEMENTED;
    }
    
    uint8_t status = inb(port STATUS);
    int timeout = wait_secs(5);
    while(!(status & 0x8) && !(status & 1)){
        if(timeout){
            return E_TIMEOUT;
        }
        timeout = wait_secs(0);
    }
    if(status & 1){
        return E_UNKNOWN;
    }
    return 0;
}
int poll_drive(uint16_t port, uint8_t term, uint8_t request){
    wait_secs(5);
    while((port & term) != request){
        if(wait_secs(0)){
            return E_TIMEOUT;
        }
    }
    return 0;
}

int ata_identify(ata_drive32_t *drive){
    uint16_t base_port = drive->base_port;
    uint16_t command_port = drive->base_port;
    outb(base_port DRIVE_HEAD, 0xA0 | drive->flags.slave);
    outb(base_port SECTOR_COUNT, 0);
    outb(base_port LBA_LOW, 0);
    outb(base_port LBA_MID, 0);
    outb(base_port LBA_HIH, 0);
    outb(base_port COMMAND, IDENTIFY);
    if(inb(base_port STATUS) == 0){
        return DE_INVALID_DRIVE;
    }
    else{
        int poll_res = poll_drive_i(base_port);
        if(poll_res){
            return poll_res;
        }
    }
    uint16_t *buffer = kmalloc(1, 7);
    padding=4;
    for(int i = 0; i < 256; i++){
        buffer[i] = inb(base_port DATA);
    }
    kprintf("        %x", (GET_SZ28(buffer)));
    return 0;
}

int ata_identify_all(){
    ata_drive32_t *master = kmalloc(1, 7);
    master->base_port = 0x1f0;
    master->command_port = 0x3f6;
    int res = ata_identify(master);
    if(res){
        return res;
    }
    return E_NO_ERR;
}