#pragma once
#include "../libs/types.h"
enum Drive_Types{
    DRIVE_NULL,
    DRIVE_VIRT,
    DRIVE_FLOPPY,
    DRIVE_PATA28,
    DRIVE_PATA48,
    DRIVE_ATAPI,
    DRIVE_SATA,
    DRIVE_USB,
};

enum Filesystem_Types{
    FS_UNFORMATTED,
    FS_FAT12,
    FS_FAT16,
    FS_FAT32,
    FS_EXT2  
};

//another example of why we don't write code at 3 am
typedef struct drive32_s{
    char number;
    char type;
    struct drive_flags{
        char read_only:1;
        char removable:1;
        char hidden:1;
        char contains_pt:1;
        char reserved:4;
    }flags;
    uint32_t size_low;
    uint32_t size_high;
}drive32_t;

typedef struct ata_drive32_s{
    drive32_t drive_s;
    uint16_t base_port;
    uint16_t command_port;
    char serial_number[20];
    struct ata_flags_s{
        uint8_t slave:1;
        uint8_t lba48:1;
    }flags;
}ata_drive32_t;


typedef struct fs32_s{
    char mountID;
    uint8_t type;
    struct fs_flags{
        uint8_t read_only:1;
        uint8_t system:1;
        uint8_t hidden:1;
        uint8_t removable:1;
        uint8_t res:4;
    }flags;
    uint32_t start_low;
    uint32_t start_high;
    uint32_t size_low;
    uint32_t size_high;
}filesystem32_t;

extern int register_drive(drive32_t *drive_to_register);
extern int get_drive_count();