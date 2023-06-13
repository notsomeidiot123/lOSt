#include "storage.h"
#include "../cpu/ecodes.h"
#include "ata.h"
#include "floppy.h"
#include <stdint.h>
#include "serial.h"
#include "../memory/mmanager.h"

char *drive_type_strs[8] = {
    "NULL  ",
    "VIRTL ",
    "FLOPPY",
    "PATA  ",
    "ATAPI ",
    "SATA  ",
    "USB   ",
};

typedef struct fat_bpb_s{
    uint8_t jshort[3];
    uint8_t oem_id[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t num_reserved_sectors;
    uint8_t fat_c;
    uint16_t root_dir_entries;
    uint16_t sectors_in_volume_small;
    uint8_t media_desc_type;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t heads;
    uint32_t lba_beginning;
    uint32_t large_sector_count;
}__attribute__((packed)) fat_bpb_t;
typedef struct fat16_ebpb_s{
    fat_bpb_t fat_bpb;
    uint8_t drive_number;
    uint8_t ntflags;
    //must be 0x28 or 0x29
    uint8_t signature;
    uint32_t volume_id;
    uint8_t volume_label[11];
    uint8_t system_id[8];
}__attribute__((packed))fat16_ebpb_t;
typedef struct fat32_ebpb_s{
    fat_bpb_t fat_bpb;
    uint32_t sectors_per_fat;
    uint16_t flags;
    uint16_t fat_version;
    uint16_t root_dir_cluster;
    //relative to partition start
    uint16_t sector_FSInfo;
    uint16_t backup_bs_sector;
    uint8_t reserved[12];
    uint8_t drive_no;
    uint8_t ntflags;
    //must be 0x28 or 0x29
    uint8_t sig;
    uint32_t volume_id;
    uint32_t volume_label[8];
}__attribute__((packed)) fat32_ebpb_t;
typedef struct fat32_fsinfo_s{
    //must be 0x41615252
    uint32_t lead_sig;
    uint8_t reserved[480];
    //must be 0x61417272
    uint32_t sig1;
    uint32_t last_free_count;
    uint32_t last_free_cluster;
    uint8_t reserved1[12];
    //must be 0xaa550000
    uint32_t trail_sig;
}__attribute__((packed)) fat32_fsinfo_t;
typedef struct fat_bs{
    filesystem32_t fs_desc;
}fat_fs_bt;
filesystem32_t *filesystems[26] = { 0 };
drive32_t *drives[26] = { 0 };

typedef struct mbr_s{
    uint8_t bootstrap[446];
    struct partition_s{
        uint8_t drive_attributes;
        uint8_t CHS_start[3];
        uint8_t partiton_type;
        uint8_t CHS_end[3];
        uint32_t partition_start;
        uint32_t size_sectors;
    }__attribute__((packed))partiton_table[4];
}__attribute__((packed)) mbr_t;

char *disk_types_ids[] = {
    "nl",
    "vd",
    "fd",
    "hd",
    "dd",
    "sd",
    "us",
};

/*
WARNING: This function allocates memory by itself... the caller is *technically*
responsible for destroying the pointer, however the intended use will most likely
result in the pointer only being destroyed (and freed) upon shudown/drive removal
*/
filesystem32_t *detect_fs(uint16_t *part_start){
    fat_bpb_t* 
}

int register_drive(drive32_t *drive_to_register){
    int mountpoint = 0;
    kprintf("[     ] Registering Drive ");
    for(int i = 0; i < 26; i++){
        if(drives[i] == 0){
            drives[i] = drive_to_register;
            drives[i]->number = i;
            mountpoint = i;
            kprintf("Number: %x Type: %s, Size: %x\r[ DONE ]\n", i, disk_types_ids[drive_to_register->type], drive_to_register->size_low);
            mbr_t *mbr = (mbr_t *)read_from_drive(kmalloc(1, 7), 1, 0, i);
            // kprintf("t e s t%x", mbr->partiton_table[0].partiton_type);
            for(int j = 0; j < 4; j++){
                padding = 0;
                kprintf("%s%c%d: LBA Address: %x, Size: %x, Type: %x, %s\n", disk_types_ids[drive_to_register->type], 'a' + i, j, mbr->partiton_table[j].partition_start, mbr->partiton_table[j].size_sectors, mbr->partiton_table[j].partiton_type, mbr->partiton_table[j].drive_attributes & 0x80 ? "BOOTABLE" : "NOT BOOTABLE");
                if(!(mbr->partiton_table[j].partiton_type == 0 || mbr->partiton_table[j].partiton_type == 0x83 || mbr->partiton_table[j].partiton_type == 0x7f)){
                    uint16_t *part_start = kmalloc(1, 7);
                    read_from_drive(part_start, 1, mbr->partiton_table[j].partition_start, i);
                    filesystem32_t *fs = detect_fs(part_start);
                }
            }
            return 0;
        }
    }
    kprintf("\r[ FAIL ]\n");
    return -1;
}

void *get_drive(int drive){
    return drives[drive];
}
uint8_t get_drive_count(){
    uint8_t count;
    for(int i = 0; i < 26; i++){
        if(drives[i]){
            count++;
        }
    }
    return count;
}
uint16_t *read_from_drive(uint16_t *buffer, int sectors, int start, int drive){
    switch(drives[drive]->type){
        case DRIVE_NULL:
            return (void*)DE_INVALID_DRIVE;
        case DRIVE_PATA28:
            return ata_read(buffer, (ata_drive32_t *)drives[drive], sectors, start);
            break;
        default:
            return (void*)E_NOT_IMPLEMENTED;
    }
    return (void*)E_NO_ERR;
}

uint16_t write_to_drive(uint16_t *buffer, int sectors, int start, int drive){
    switch(drives[drive]->type){
        case DRIVE_NULL:
            return DE_INVALID_DRIVE;
        case DRIVE_PATA28:
            ata_write(buffer, (ata_drive32_t *)drives[drive], sectors, start);
            break;
        default:
            return E_NOT_IMPLEMENTED;
    }
    return E_NO_ERR;
}



int register_fs(filesystem32_t *filesystem){
    for(int i = 0; i < 26; i++){
        if(filesystems[i] == 0){
            filesystems[i] = filesystem;
            filesystems[i]->mountID = i;
            return 0;
        }
    }
    return -1;
}

int fwrite_at(FILE* f, char *buffer, uint32_t size){
    return 0;
}

FILE *fopen(char *name, int mode){
    return 0;
}
int fwrite(FILE* f, char *buffer, uint32_t size){
    return 0;
}
uint8_t *fread_at(FILE *f, uint8_t* buffer, uint32_t size, uint32_t start){
    return 0;
}
uint8_t *fread(FILE *f, uint8_t* buffer, uint32_t size){
    return 0;
}
