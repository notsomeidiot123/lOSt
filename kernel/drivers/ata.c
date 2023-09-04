//change read/write to be loaded as helper processes with highest priority when we switch back to multitasking
#include "ata.h"
#include "storage.h"
#include "../memory/mmanager.h"
#include "../cpu/io.h"
#include "../graphics/vga.h"
#include "../cpu/idt.h"
#include "../cpu/ecodes.h"
#include "../memory/string.h"
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
// int poll_drive(uint16_t port, uint8_t term, uint8_t request){
//     wait_secs(5);
//     while((port & term) != request){
//         if(wait_secs(0)){
//             return E_TIMEOUT;
//         }
//     }
//     return 0;
// }

//restructre to not require master drive
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
        buffer[i] = inw(base_port DATA);
    }
    // drive->flags.lba48 = IS_LBA48(buffer) ? 1 : 0;
    drive->drive_s.flags.removable = 0;
    drive->drive_s.bytes_per_sector = 512;
    if(IS_LBA48(buffer)){
        drive->drive_s.size_high = GET_SZ48H(buffer);
        drive->drive_s.size_low = GET_SZ48L(buffer);
        drive->drive_s.type = DRIVE_PATA28;
    }
    else{
        drive->drive_s.size_low = GET_SZ28(buffer);
        drive->drive_s.type = DRIVE_PATA28;
    }
    register_drive((drive32_t * )drive);
    kfree(buffer);
        
    outb(base_port DRIVE_HEAD, 0xB0 | drive->flags.slave);
    outb(base_port SECTOR_COUNT, 0);
    outb(base_port LBA_LOW, 0);
    outb(base_port LBA_MID, 0);
    outb(base_port LBA_HIH, 0);
    outb(base_port COMMAND, IDENTIFY);
    if(inb(base_port STATUS) == 0){
        return 0;
    }
    ata_drive32_t* slave = kmalloc(1, 7);
    clearmem(slave, sizeof(ata_drive32_t));
    slave->base_port = drive->base_port;
    slave->command_port = drive->command_port;
    slave->flags.slave = 1;
    buffer = kmalloc(1, 7);
    for(int i = 0; i < 256; i++){
        buffer[i] = 0;
    }
    for(int i = 0; i < 256; i++){
        buffer[i] = inw(base_port DATA);
    }
    // slave->flags.lba48 = IS_LBA48(buffer) ? 1 : 0;
    slave->drive_s.flags.removable = 0;
    slave->drive_s.bytes_per_sector = 512;
    if(IS_LBA48(buffer)){
        slave->drive_s.size_high = GET_SZ48H(buffer);
        slave->drive_s.size_low = GET_SZ48L(buffer);
        slave->drive_s.type = DRIVE_PATA28;
    }
    else{
        slave->drive_s.size_low = GET_SZ28(buffer);
        slave->drive_s.type = DRIVE_PATA28;
    }
    register_drive((drive32_t *)slave);
    // kprintf("%x", slave->drive_s.size_low);
    return 0;
}

int ata_identify_all(){
    ata_drive32_t *master = kmalloc(1, 7);
    clearmem(master, sizeof(ata_drive32_t));
    master->base_port = 0x1f0;
    master->command_port = 0x3f6;
    int res = ata_identify(master);
    if(res){
        return res;
    }
    return E_NO_ERR;
}

void wait_ready(ata_drive32_t *drive){
    while(!(inb(drive->base_port STATUS) & 0x40));
}

int poll_drive(ata_drive32_t *drive){
    uint8_t res = inb(drive->base_port STATUS);
    wait_secs(5);//wait 5 seconds, try 6 times, 5*6 = 30 for maximum spin-up time, afterwards, software reset and retry
    //as default
    while(!(res & 0x40 || !(res & 0x8)) || res & 0x80 && !wait_secs(0)){
        if(res & 0x20 || res & 1){
            kprintf("ERROR: %x\n", inb(drive->base_port ERROR));
            return DE_DRIVE_ERR;
        }
        res = inb(drive->base_port STATUS);
    }
    if(wait_secs(0)){
        // software_reset()
        // kprintf("DRIVE ERROR!\n");
        return E_TIMEOUT;
    }
    return E_NO_ERR;
}



uint16_t *ata28_read(uint16_t *buffer, ata_drive32_t *drive, uint32_t sectors, uint32_t start){
    wait_ready(drive);
    outb(drive->base_port DRIVE_HEAD, 0xE0 | (drive->flags.slave << 4) | ((sectors >> 24) & 0xf));
    // wait_ready(drive);
    outb(drive->base_port ERROR, 0);
    outb(drive->base_port SECTOR_COUNT, sectors);
    outb(drive->base_port LBA_LOW, start & 0x0000ff);
    outb(drive->base_port LBA_MID, (start & 0x00ff00) >> 8);
    outb(drive->base_port LBA_HIH, (start & 0xff0000) >> 16);
    outb(drive->base_port COMMAND, 0x20);
    wait_ready(drive);
    for(int j = 0; j < sectors; j++){
        int trycount = 0;
        int trymax = 6;//replace with settings later
        int res = poll_drive(drive);
        while(trycount < trymax && res){
            if(res){
                res = poll_drive(drive);
                trycount++;
            }
            else{
                break;
            }
        }
        if(res){
            return 0;
        }
        for(int i = 0; i < 256; i++){
            uint16_t data = inw(drive->base_port DATA);
            buffer[(j * 256)+ i] = data;
            // poll_drive(drive);
        }
    }
    return buffer;
}

uint16_t *ata_read(uint16_t *buffer, ata_drive32_t *drive, uint32_t sectors, uint32_t start){
    if(drive->flags.lba48){
        return (uint16_t *) E_NOT_IMPLEMENTED;
    }
    else{
        long end = 0;
        for(int i = 0; i < sectors / 256; i++){
            end += (256 * 256);
            ata28_read(buffer + end, drive, 0, start + (i * 256));
        }
        /*MODIFY ALL 512S TO USE THE DRIVE'S SECTOR SIZE*/
        ata28_read(buffer + end, drive, sectors % 256, start + end / 512);
    }
    return buffer;
}
uint16_t ata28_write(uint16_t *buffer, ata_drive32_t *drive, uint32_t sectors, uint32_t start){
    wait_ready(drive);
    int stat = inb(drive->base_port STATUS);
    wait_secs(5);
    while(stat & 0x80 && !(stat & 0x8)){
        stat = inb(drive->base_port STATUS);
        if(stat & 1 || stat & 0x20 || wait_secs(0)){
            return -1;
        }
    }
    outb(drive->base_port DRIVE_HEAD, 0xE0 | (drive->flags.slave << 4) | ((sectors >> 24) & 0xf));
    // wait_ready(drive);
    kprintf("Sector: %x, Sector count: %d", start, sectors);
    outb(drive->base_port ERROR, 0);
    outb(drive->base_port SECTOR_COUNT, sectors);
    outb(drive->base_port LBA_LOW, start & 0x0000ff);
    outb(drive->base_port LBA_MID, (start & 0x00ff00) >> 8);
    outb(drive->base_port LBA_HIH, (start & 0xff0000) >> 16);
    outb(drive->base_port COMMAND, 0x30);
    wait_ready(drive);
    for(int j = 0; j < sectors; j++){
        // int trycount = 0;
        // int trymax = 6;//replace with settings later
        // int res = poll_drive(drive);
        // while(trycount < trymax && res){
        //     if(res){
        //         res = poll_drive(drive);
        //         trycount++;
        //     }
        //     else{
        //         break;
        //     }
        // }
        // if(res){
        //     return 0;
        // }
        for(int i = 0; i < 256; i++){
            outw(drive->base_port DATA, buffer[(j * 256)+ i]);
            
            // poll_drive(drive);
        }
    }
    outb(drive->base_port COMMAND, 0xe7);
    return 0;
}

// uint16_t ata28_write(uint16_t *buffer, ata_drive32_t *drive, uint32_t sectors, uint32_t start){
//     wait_ready(drive);
//     outb(drive->base_port DRIVE_HEAD, 0xE0 | (drive->flags.slave << 4) | ((sectors >> 24) & 0xf));
//     // wait_ready(drive);
//     outb(drive->base_port ERROR, 0);
//     outb(drive->base_port SECTOR_COUNT, sectors);
//     outb(drive->base_port LBA_LOW, start & 0xff);
//     outb(drive->base_port LBA_MID, (start & 0xff00) >> 8);
//     outb(drive->base_port LBA_HIH, (start & 0xff0000) >> 16);
//     outb(drive->base_port COMMAND, 0x30);
//     wait_ready(drive);
//     for(int j = 0; j < sectors; j++){
//         int trycount = 0;
//         int trymax = 5;//replace with settings later
//         int res = poll_drive(drive);
//         while(trycount < trymax && res){
//             if(res){
//                 res = poll_drive(drive);
//                 trycount++;
//             }
//             else{
//                 break;
//             }
//         }
//         if(res){
//             return res;
//         }
//         for(int i = 0; i < 256; i++){
//             outw(drive->base_port DATA, buffer[(j * 256) + i]);
//             kprintf("Data port: 0x%x", drive->base_port);
//             outb(0x80, 0); //very short delay
//             poll_drive(drive);
//         }
//     }
//     wait_ready(drive);
//     //0xE7 = flush cache
//     outb(drive->base_port COMMAND, 0xE7);
//     return E_NO_ERR;
// }

uint16_t ata_write(uint16_t *buffer, ata_drive32_t *drive, uint32_t sectors, uint32_t start){
    if(drive->flags.lba48){
        return  E_NOT_IMPLEMENTED;
    }
    else{
        long end = 0;
        for(int i = 0; i < sectors / 256; i++){
            end += (256 * 256);
            ata28_write(buffer + end, drive, 0, start + (i * 256));
        }
        /*MODIFY ALL 512S TO USE THE DRIVE'S SECTOR SIZE*/
        ata28_write(buffer + end, drive, sectors % 256, start + end / 512);
    }
    return E_NO_ERR;
}