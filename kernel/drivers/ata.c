//change read/write to be loaded as helper processes with highest priority when we switch back to multitasking
#include "storage.h"
#include "../cpu/io.h"
#include "../graphics/vga.h"
#include "../cpu/idt.h"
#include "timer.h"
// #include "../"

#define ATA_DATA_R_P 0x1f0
#define ATA_ERRORR_P 0x1f1
#define ATA_FEATES_P 0x1f1
#define ATA_SECOUT_P 0x1f2
#define ATA_LBALOW_P 0x1f3
#define ATA_LBAMID_P 0x1f4
#define ATA_LBAHIH_P 0x1f5
#define ATA_DRVSEL_P 0x1f6
#define ATA_STATUS_P 0x1f7
#define ATA_COMMND_P 0x1f7
#define ATA_ALT_ST_P 0x3f6
#define ATA_DEVCNT_P 0x3f6
#define ATA_DRVADD_P 0x3f7
//primary drives, implement secondary drive later

unsigned short ata_id_primary_master[256];

typedef struct{
    char *name;
    unsigned int size_sectors_low;
    unsigned int size_sectors_high;
    unsigned short serial_number[11];
    unsigned short model_number[21];
    unsigned int drive_num;
    unsigned char perms_usr : 3;
    unsigned char perms_sys : 3;
    unsigned char lock : 1;
    unsigned char hidden : 1;
    unsigned char perms_adm : 3;
    unsigned char lba_28_48 : 1;
}ata_drive;

ata_drive ata_drives[4];
int ata_identify(){
    outb(ATA_DRVSEL_P, 0xa0);
    outb(ATA_SECOUT_P, 0);
    outb(ATA_LBALOW_P, 0);
    outb(ATA_LBAMID_P, 0);
    outb(ATA_LBAHIH_P, 0);
    outb(ATA_COMMND_P, 0xec);
    unsigned cl = inb(ATA_LBAMID_P);
    unsigned ch = inb(ATA_LBAHIH_P);
    
    if(cl == 0x14 && ch == 0xeb){
        return 1;
    }
    else if(cl == 0x3c && ch == 0xc3){
        return 2;
    }
    
    if(!inb(ATA_STATUS_P)){
        kputs("ERROR 0X0023: ATA IDENTIFY ERROR\n");
        return -1;
    }
    else{
        char c = inb(ATA_STATUS_P);
        while(c & 0x80 && c & 8 && !(c & 1)){
            if(inb(ATA_LBAMID_P) || inb(ATA_LBAHIH_P)){
                kputs("ERROR 0X0023: ATA IDENTIFY ERROR\n");
                return -1;
            }
        }
        if(c & 1){
            kputs("ERROR 0X0023: ATA IDENTIFY ERROR\n");
            return -1;
        }
        else{
            for(int i = 0; i < 256; i++){
                ata_id_primary_master[i] = inw(ATA_DATA_R_P);
                if(i > 9 && i < 20) ata_drives[0].serial_number[i-10] = ata_id_primary_master[i];
                if(i > 26 && i < 47) ata_drives[0].model_number[i-27] = ata_id_primary_master[i];
            }
        }
    }
    
    ata_drives[0].drive_num = 0;
    ata_drives[0].name = "Boot";
    ata_drives[0].perms_usr = 7;
    ata_drives[0].perms_adm = 7;
    ata_drives[0].perms_sys = 7;
    ata_drives[0].lba_28_48 = ata_id_primary_master[83] & 1 << 10;
    if((ata_id_primary_master[60] | ata_id_primary_master[61]<<16) == 0){
        ata_drives[0].size_sectors_high = ata_id_primary_master[102] | ata_id_primary_master[102] << 16;
        ata_drives[0].size_sectors_low = ata_id_primary_master[100] | ata_id_primary_master[101] << 16;
        
    }
    else{
        ata_drives[0].size_sectors_low = ata_id_primary_master[60] | ata_id_primary_master[61]<<16;
        ata_drives[0].size_sectors_high = 0;
    }
    //[60] is low, [61] is high
    return 0;
}

int ata_int_recieved = 0;

int ata_software_reset(){
    outb(ATA_DEVCNT_P, 6);
    outb(ATA_DEVCNT_P, 2);
    return 0;
}


int ata_select_drive(char drive_no){
    outb(ATA_DRVADD_P, 1);
    outb(ATA_DEVCNT_P, 0);
    return 0;
}

void ata_irq_handler(irq_registers_t *r){
    ata_int_recieved = 1;
    // kputc(0x41);
}
int ata_retry_cap = 2;
int ata_read28(unsigned short *dest, unsigned int lba_addr, unsigned char drive_no, char sector_count){
    if(drive_no > 1) return -1;
    // ata_select_drive_p(drive_no);
    wait_secs(15);
    int trycount = 0;
    while(inb(ATA_STATUS_P) & 0x80 && !(inb(ATA_STATUS_P) & 0x8)){
        if(wait_secs(0)){
            ata_software_reset();
            wait_secs(15);
            trycount++;
        }
        if(trycount >= ata_retry_cap){
            kputs("ERROR 0x0021: ATA TIMEOUT\n");
            return -1;
        }
    }
    // if(inb(ATA_STATUS_P & 1)){
    //     if(textmode_active){
    //         tm_kputs("ERROR 0x0022: ATA ERROR\n", T_ERROR);
    //     }
    //     ata_software_reset();
    //     return -1;
    // }
    ata_int_recieved = 0;
    outb(ATA_DRVSEL_P, 0xe0 | drive_no << 4 | (lba_addr >> 24 & 0xf));
    outb(ATA_ERRORR_P, 0);
    outb(ATA_SECOUT_P, sector_count);
    
    outb(ATA_LBALOW_P, (unsigned char) (lba_addr));
    outb(ATA_LBAMID_P, (unsigned char) (lba_addr >> 8));
    outb(ATA_LBAHIH_P, (unsigned char) (lba_addr >> 16));
    outb(ATA_COMMND_P, 0x20);
    while(inb(ATA_STATUS_P) & 0x80 | !(inb(ATA_STATUS_P) & 8));
    for(int i = 0; i < sector_count+1; i++){
        for(int j = 0; j < 256; j++){
            dest[j] = inw(ATA_DATA_R_P);
        }
        dest += 512;
    }
    outb(ATA_COMMND_P, 0xe7);
    return 0;
}

int ata_write28(unsigned short *src, unsigned int lba_addr, unsigned char drive_no, char sector_count){
    if(drive_no > 1) return -1;
    wait_secs(15);
    int trycount = 0;
    while(inb(ATA_STATUS_P) & 0x80 && !(inb(ATA_STATUS_P) & 0x8)){
        if(wait_secs(0)){
            ata_software_reset();
            wait_secs(15);
            trycount++;
        }
        if(trycount == ata_retry_cap){
            kputs("ERROR 0x0021: ATA TIMEOUT\n");
            return 0;
        }
    }
    // if(inb(ATA_STATUS_P & 1)){
    //     if(textmode_active){
    //         ata_software_reset();
    //         tm_kputs("ERROR 0x0022: ATA ERROR\n", T_ERROR);
    //     }
    //     return -1;
    // }
    ata_int_recieved = 0;
    outb(ATA_DRVSEL_P, 0xe0 | drive_no << 4 | (lba_addr >> 24 & 0xf));
    outb(ATA_ERRORR_P, 0);
    outb(ATA_SECOUT_P, (unsigned char) sector_count);
    
    
    outb(ATA_LBALOW_P, (unsigned char) lba_addr);
    outb(ATA_LBAMID_P, (unsigned char) (lba_addr >> 8));
    outb(ATA_LBAHIH_P, (unsigned char) (lba_addr >> 16));
    outb(ATA_COMMND_P, 0x30);
    
    while(inb(ATA_STATUS_P) & 0x80 | !(inb(ATA_STATUS_P) & 8));
    
    if(inb(ATA_ERRORR_P)) return -1;
    for(int i = 0; i < sector_count; i++){
        for(int j = 0; j < 256; j++){
            outw(ATA_DATA_R_P, src[j]);
            for(int i =0; i < 1; i++);
        }
        src += 512;
    }
    outb(ATA_COMMND_P, 0xe7);
    return 0;
}