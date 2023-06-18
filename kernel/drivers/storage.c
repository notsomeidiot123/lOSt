#include "storage.h"
#include "../cpu/ecodes.h"
#include "ata.h"
#include "floppy.h"
#include "../memory/string.h"
#include <stdint.h>
#include "serial.h"
#include "../memory/mmanager.h"

#define EXT_SIG 0xef53

char *drive_type_strs[8] = {
    "NULL  ",
    "VIRTL ",
    "FLOPPY",
    "PATA  ",
    "ATAPI ",
    "SATA  ",
    "USB   ",
};

char *fs_type_strs[] = {
    "NONE",
    "FAT12",
    "FAT16",
    "FAT32",
    "EXT2",
};
//EXT3 will not be supported. EXT3 will probably never be supported
//Why am i dedicating so much effort into something that will never be used lmao
typedef struct fat_bpb_s{
    uint8_t jshort[3];
    uint8_t oem_id[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t num_reserved_sectors;
    uint8_t fat_c;
    uint16_t root_dir_entries;
    uint16_t sectors_in_vol;
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
    uint16_t boot_magic;
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
fs_fat16_t *register_fat16(filesystem32_t *fs, int drive, uint16_t *buffer){
    int id = 0;
    for(int i = 0; i < 26; i++){
        if(filesystems[i] == 0){
            id = i;
        }
    }
    fat_bpb_t* fat_bpb = (fat_bpb_t *)buffer;
    if(fat_bpb->bytes_per_sector == 0 || fat_bpb->sectors_per_cluster == 0){
        return 0;
    }
    uint32_t fat_size = fat_bpb->sectors_per_fat;
    uint32_t root_dir_sectors = ((fat_bpb->root_dir_entries * 32 ) + (fat_bpb->bytes_per_sector - 1)) / fat_bpb->bytes_per_sector == 0;
    uint32_t data_sectors = (fat_bpb->sectors_in_vol == 0 ? fat_bpb->large_sector_count : fat_bpb->sectors_in_vol) - (fat_bpb->num_reserved_sectors + fat_bpb->fat_c * fat_size + root_dir_sectors);
    uint32_t total_clusters = data_sectors/fat_bpb->sectors_per_cluster;
    fs_fat16_t *fat = (fs_fat16_t*)fs;
    fat->fs_base.drive = drive;
    fat->fs_base.mountID = id;
    fat->fs_base.flags = (struct fs_flags){0};
    fat->fs_base.size_low = fat_bpb->sectors_in_vol == 0 ? fat_bpb->large_sector_count : fat_bpb->sectors_in_vol;
    fat->first_data_sector = fat_bpb->num_reserved_sectors + (fat_bpb->fat_c * fat_bpb->sectors_per_fat);
    fat->sectors_per_cluster = fat_bpb->sectors_per_cluster;
    fat->sectors_per_fat = fat_bpb->sectors_per_fat;
    fat->fat_offset_primary = fat_bpb->num_reserved_sectors;
    fat->fat_offset_secondary = fat_bpb->num_reserved_sectors + fat_bpb->sectors_per_fat;
    fat->total_clusters = total_clusters;
    fat->free_clusters = total_clusters;
    
}

filesystem32_t *detect_fs(uint16_t *part_start, int drive, uint32_t start_sector){
    
    char ext = part_start[512 + (56/2)] == EXT_SIG;
    if(ext){
        //this is a whole other beast...
    }
    else{
        fat_bpb_t* fat_bpb = (fat_bpb_t *)part_start;
        if(fat_bpb->bytes_per_sector == 0 || fat_bpb->sectors_per_cluster == 0){
            return 0;
        }
        uint32_t fat_size = (fat_bpb->sectors_per_fat == 0) ? ((fat32_ebpb_t *)fat_bpb)->sectors_per_fat : fat_bpb->sectors_per_fat;
        uint32_t root_dir_sectors = ((fat_bpb->root_dir_entries * 32 ) + (fat_bpb->bytes_per_sector - 1)) / fat_bpb->bytes_per_sector == 0;
        uint32_t data_sectors = (fat_bpb->sectors_in_vol == 0 ? fat_bpb->large_sector_count : fat_bpb->sectors_in_vol) - (fat_bpb->num_reserved_sectors + fat_bpb->fat_c * fat_size + root_dir_sectors);
        uint32_t total_clusters = data_sectors/fat_bpb->sectors_per_cluster;
        filesystem32_t *fs = kmalloc(1, 6);
        fs->start_low = start_sector;
        //if the first two clusters are all 0's, it's EXT, else, it's NTFS or FAT12/16/32 (screw NTFS)
        fs->type = total_clusters <= 0xfff ? FS_FAT12: total_clusters <= 0xffff ? FS_FAT16 : FS_FAT32;
        switch(fs->type){
            case FS_FAT12:
                //fall through to fat16 cause they are pretty much the same
            case FS_FAT16:
                //fat16 sepcific things... i guess;
                
                break;
            case FS_FAT32:
                //fat32 specific things, this time, do something ***slightly*** different
            break;
            default:
                //how did we get here?
                break;
        }
    }
    
    return 0;
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
                    read_from_drive(part_start, 8, mbr->partiton_table[j].partition_start, i);
                    filesystem32_t *fs = detect_fs(part_start, i, mbr->partiton_table[j].partition_start);
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
