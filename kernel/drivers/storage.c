#include "storage.h"
#include "../cpu/ecodes.h"
#include "ata.h"
#include "floppy.h"
#include "../memory/string.h"
#include <stdint.h>
#include "serial.h"
#include "../memory/mmanager.h"
#include "../filesystems/fat.h"


#define EXT_SIG 0xef53

const char *drive_type_strs[8] = {
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

const char *disk_types_ids[] = {
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
filesystem32_t *get_fs(int fs){
    return filesystems[fs];
}

filesystem32_t *detect_fs(uint16_t *part_start, int drive, uint32_t start_sector, mbr_t *mbr, uint8_t partition){
    int id = 0;
    for(int i = 0; i < 26; i++){
        if(filesystems[i] == 0){
            id = i;
            break;
        }
    }
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
        filesystem32_t *check = fs;
        fs->size_low = mbr->partiton_table[partition].size_sectors;
        fs->partition = partition;
        fs->start_low = start_sector;
        fs->mountID = id;
        fs->type = total_clusters <= 0xfff ? FS_FAT12: total_clusters <= 0xffff ? FS_FAT16 : FS_FAT32;
        switch(fs->type){
            case FS_FAT12:
                //fall through to fat16 cause they are pretty much the same
            case FS_FAT16:
                //fat16 sepcific things... i guess;
                // fs = (filesystem32_t *)register_fat16(fs, drive, part_start);
                break;
            case FS_FAT32:
                //fat32 specific things, this time, do something ***slightly*** different
                fs = (filesystem32_t *)fat32_register(fs, drive, part_start, start_sector);
            break;
            default:
                //how did we get here?
                break;
        }
        if(fs != check){
            kfree(check);
        }        
        // fs->type = FS_FAT;
        filesystems[id] = fs;
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
                    filesystem32_t *fs = detect_fs(part_start, i, mbr->partiton_table[j].partition_start, mbr, j);
                    kfree(part_start);
                }
            }
            return 0;
        }
    }
    kprintf("\r[ FAIL ]\n");
    return -1;
}

drive32_t *get_drive(int drive){
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

//WARNING: FOPEN ALLOCATES MEMORY
FILE *fopen(char *name, int mode){
    if(!name){
        return 0;
    }
    switch(filesystems[name[0]- 'A']->type){
        case FS_FAT16:
        case FS_FAT32:
            return (FILE *)fat32_open_file(name + 2, (fs_fat_t *)filesystems[name[0] - 'A'], mode);
        break;
        case FS_FAT:
            // fat_open_file(name, (fs_fat_t*)filesystems[name[0]-'A'], mode);
            break;
        default:
            break;
    }
    return 0;
}

int fwrite(FILE* file, uint8_t *buffer, uint32_t size){
    
};

void fread(FILE* f, uint8_t* buffer, uint32_t size){
    
}
