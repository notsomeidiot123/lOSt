//change read/write to be loaded as helper processes with highest priority when we switch back to multitasking
#include "storage.h"
#include "../cpu/io.h"
#include "../graphics/vga.h"
#include "../cpu/idt.h"
#include "../cpu/ecodes.h"
#include "timer.h"

#define DATA + 0
#define ERROR + 1
#define FEATURES + 1
#define SECTOR_COUNT + 2
#define SECTOR_NUMBER + 3
#define CYLINDER_LOW + 4
#define CYLINDER_HIGH + 5
#define DRIVE_HEAD + 6
#define STATUS + 7
#define COMMANd + 7

#define ALT_STATUS + 0
#define DEVICE_CONTROL + 0
#define DRIVE_ADDRESS + 1


#define SLAVE_DRIVE 0xF0
#define MASTER_DRIVE 0xE0

#define IS_ATA(a) a[0] & 0x8000
#define GET_SZ28(a) a[60] | (a[61] << 8)
#define IS_LBA48(a) a[83] & (1 << 10)
#define GET_SZ48L(a) a[100] | (a[101] << 8)
#define GET_SZ48H(a) a[102] | (a[103] << 8)



int ata_identify_all(){
    
    return E_NO_ERR;
}