#include "../drivers/storage.h"
#include "../memory/mmanager.h"
#include "../memory/string.h"
#include "fat.h"
#include "../drivers/serial.h"


#define BYTES_PER_FAT_ENTRY_EX 4
#define BYTES_PER_FAT_ENTRY_32 4
#define BYTES_PER_FAT_ENTRY_16 2
#define BYTES_PER_FAT_ENTRY_12 2

void cache_fat32(fs_fat_t *fat, uint32_t start){
    //woo that took me a while to re-read
    //remember to replace 128 with bytes_per_sector/4 (four bytes per FAT entry)
    uint16_t *buffer;
    buffer = fat->fat_cache == 0 ? kmalloc(fat->fat_cache_size/8 + 1, 6) : fat->fat_cache;
    if(!buffer){
        kprintf("\e[lOSt] Error allocating memory for Fat Cache!\n");
    }
    fat->cached_clusters_start = start;//                       Calculates the start sector offset from the first fat cluster by dividing the start by how many entries fit in a sector
    read_from_drive(buffer, fat->fat_cache_size, start/(fat->fs_base.sector_size_bytes/4) + fat->fat_offset_primary, fat->fs_base.drive);
    //example: start = entry 72, 72/(512/4) = 0; sector_to_read = fat_start
}
void write_cache(fs_fat_t* fat){
    write_to_drive((uint16_t *)fat->fat_cache, fat->fat_cache_size, fat->fat_offset_primary + fat->cached_clusters_start, fat->fs_base.drive);
}


//Searches Dir for file matching filename file
//  If found, returns the ptr to the dirent for file
//returns null on faliure
dirent_t *search_dir(char *file, dirent_t *dir, uint32_t dirsize){
    char *name = file;
    char *extension = file;
    while(*extension != '.'){
        if(!*extension){
            extension = 0;
            break;
        }
        extension++;
    }
    for(int i = 0; i < dirsize; i++){
        if(!kmemcmp(name, dir[i].name, extension - name)){
            if(extension){
                if(kmemcmp(extension, dir[i].name, kstrlen(extension))){
                    continue;
                }
            }
            kprintf("Returning found file!\n");
            return &dir[i];
        }
        
    }
    kprintf("end search\n");
    return 0;
}

uint32_t get_next_cluster32(uint32_t cluster, fs_fat_t *fat){
    //i feel like this is broken... we'll see later
    if(cluster < fat->cached_clusters_start/fat->fs_base.sector_size_bytes/BYTES_PER_FAT_ENTRY_32 || cluster > fat->cached_clusters_start/fat->fs_base.sector_size_bytes/BYTES_PER_FAT_ENTRY_32 + fat->cached_clusters_size){
        cache_fat32(fat, cluster);
    }
    return fat->fat_cache[cluster - fat->cached_clusters_start/fat->fs_base.sector_size_bytes/BYTES_PER_FAT_ENTRY_32];
}
void set_next_cluster32(uint32_t cluster, uint32_t next, fs_fat_t *fat){
    if(cluster < fat->cached_clusters_start/fat->fs_base.sector_size_bytes/BYTES_PER_FAT_ENTRY_32 || cluster > fat->cached_clusters_start/fat->fs_base.sector_size_bytes/BYTES_PER_FAT_ENTRY_32 + fat->cached_clusters_size){
        write_cache(fat);
        cache_fat32(fat, cluster);
    }
    if(cluster == next){
        next = -1;
    }
    fat->fat_cache[cluster - fat->cached_clusters_start/fat->fs_base.sector_size_bytes/BYTES_PER_FAT_ENTRY_32] = next;
}

FAT_FILE *fat32_create_file(){

}

int fat32_read(FILE *file, int size, char *buffer, fs_fat_t *fat){
    if(!file){
        return -1;
    }
    else if(!buffer){
        return -2;
    }
    else if(!fat){
        return -3;
    }
    FAT_FILE *fatf = (FAT_FILE *)file;
    // read_from_drive((uint16_t *)buffer, , fat->first_data_sector + (fatf->current_cluster * 8), fat->fs_base.drive);
    int sectors = ((size / fat->fs_base.sector_size_bytes) + 1) * fat->sectors_per_cluster;
    uint32_t cluster = fatf->current_cluster;
    while(sectors > 0){
        read_from_drive((uint16_t *)buffer, fat->sectors_per_cluster, fat->first_data_sector + (cluster * fat->sectors_per_cluster), fat->fs_base.drive);
        cluster = get_next_cluster32(cluster, fat);
        if(cluster >= 0x0FFFFFF8){
            break;
        }
        sectors /= fat->sectors_per_cluster;
    }
    return 0;
}

// filename should be truncated to the first slash
// for example, the first call, to fopen("A:/folder/file.txt", "WR") should call this function as
// fat32_open_file("/folder/file.txt", fs, 6);
FAT_FILE *fat32_open_file(char *filename, fs_fat_t* fat, uint8_t mode){
    if(!fat){
        return 0;
    }
    else if(!filename){
        return 0;
    }
    char *str_to_tok = kmalloc((kstrlen(filename)/4096) + 1, 6);
    kmemcpy(filename, str_to_tok, kstrlen(filename));
    char *token = kstrtok(str_to_tok, "/");
    FAT_FILE *dir = &fat->root_file;
    while(token){

        fat32_read(dir, -1, char *buffer, fs_fat_t *fat)
        dir = search_dir(char *file, dirent_t *dir, uint32_t dirsize)
        token = kstrtok(0, "/");
    }
}

fs_fat_t *fat32_register(filesystem32_t *fs, int drive, uint16_t *buffer){
    fat32_ebpb_t *fat_boot = (fat32_ebpb_t *)buffer;
    fs_fat_t *fatfs = (fs_fat_t *)fs;
    fat32_fsinfo_t *fsinfo = kmalloc(8, 6);
    if(!read_from_drive((void *)fsinfo, 7, fat_boot->sector_FSInfo + fat_boot->fat_bpb.lba_beginning, drive)){
        kprintf("\e[lOSt] Error mounting FAT Partition: Error Reading from Disk\n");
        return 0;
    }

    //populate the rest of the filesystem struct with data from here
    uint32_t fat_size = fat_boot->sectors_per_fat;
    fatfs->first_data_sector = (fat_boot->fat_bpb.fat_c * fat_size);
    fatfs->fat_offset_primary = (fat_boot->fat_bpb.num_reserved_sectors);
    fatfs->sectors_per_fat = fat_size;
    uint32_t data_sectors = fat_boot->fat_bpb.sectors_in_vol - (fat_boot->fat_bpb.num_reserved_sectors + (fat_boot->fat_bpb.fat_c * fat_size));
    fatfs->total_clusters = data_sectors / fat_boot->fat_bpb.sectors_per_cluster;
    fatfs->fs_base.sector_size_bytes = fat_boot->fat_bpb.bytes_per_sector;
    fatfs->root_dir_cluster = fat_boot->root_dir_cluster;
    fatfs->fat_cache_size = fat_boot->fat_bpb.sectors_per_fat < 2048 ? fat_boot->fat_bpb.sectors_per_fat : 2048;
    if(fat_boot->fat_bpb.fat_c > 1){
        fatfs->fat_offset_secondary = fat_boot->fat_bpb.num_reserved_sectors + fat_size;
    }
    fatfs->root_file.start_cluster = fatfs->root_dir_cluster;
    fatfs->root_file.current_cluster = fatfs->root_dir_cluster;
    // fatfs->root_file.dirent = (dirent_t ){0};
    fatfs->root_file.file_base.fs_type = FS_FAT32;
    fatfs->root_file.file_base.mode = 6;
    fatfs->root_file.file_base.permission = 6;
    fatfs->root_file.file_base.position = 0;
    return fatfs;
}
