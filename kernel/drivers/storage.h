#pragma once
#include "../libs/types.h"
enum Drive_Types{
    DRIVE_NULL,
    DRIVE_VIRT,
    DRIVE_FLOPPY,
    DRIVE_PATA28,
    DRIVE_ATAPI,
    DRIVE_SATA,
    DRIVE_USB,
};
enum Filesystem_Types{
    FS_UNFORMATTED,
    FS_FAT = 1,
    FS_FAT12 = 1,
    FS_FAT16 = 3,
    FS_FAT32 = 5,
    FS_EXT2  
};
extern const char *disk_types_ids[];
extern const char *drive_type_strs[8];
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
    uint32_t bytes_per_sector;
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
    //populated by detect_fs()
    char mountID;
    //populated by detect_fs()
    uint8_t type;
    //populated by detect_fs()
    uint8_t partition;
    uint8_t drive;
    struct fs_flags{
        uint8_t read_only:1;
        uint8_t system:1;
        uint8_t hidden:1;
        uint8_t removable:1;
        uint8_t res:4;
    }flags;
    //populated by detect_fs()
    uint32_t start_low;
    //populated by detect_fs();
    uint32_t start_high;
    //populated by the filesystem driver
    uint32_t sector_size_bytes;
    //populated by detect_fs();
    uint32_t size_low;
    uint32_t size_high;
}filesystem32_t;
enum FILE_MODES{
    MODE_WRITE = 1,
    MODE_READ = 2
};

typedef struct file_s{
    uint8_t permission;
    uint8_t mode;
    uint8_t fs_type;
    uint32_t position;
}FILE;

extern int register_drive(drive32_t *drive_to_register);
extern uint8_t get_drive_count();
extern uint16_t *read_from_drive(uint16_t *buffer, int sectors, int start, int drive);
extern uint16_t write_to_drive(uint16_t *buffer, int sectors, int start, int drive);
extern drive32_t *get_drive(int drive);
extern filesystem32_t *get_fs(int fs);
extern int register_fs(filesystem32_t *fs);

//WARNING: FOPEN ALLOCATES MEMORY
extern FILE *fopen(char *name, int mode);
extern int fwrite(FILE* file, uint8_t *buffer, uint32_t size);
// extern int fwrite_at(char *filename, char *buffer, uint32_t size);
extern void fread(FILE* f, uint8_t* buffer, uint32_t size);
// extern uint8_t *fread_at(FILE *f, uint8_t* buffer, uint32_t size, uint32_t start);