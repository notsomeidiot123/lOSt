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
            return &dir[i];
        }
        
    }
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
uint32_t fat32_find_free_cluster(fs_fat_t *fat){
    int c = fat->last_free_cluster;
    for(int i = fat->last_free_cluster == -1 ? 0 : fat->last_free_cluster; i < fat->total_clusters; i++){
        if(!get_next_cluster32(i, fat)){
            c = i;
            break;
        }
    }
    fat->last_free_cluster = c;
    return c;
}
int fat32_write(FILE *file, int size, void *buffer, fs_fat_t *fat){
    FAT_FILE *ffile = (FAT_FILE*)file;
    uint32_t cluster = ffile->start_cluster;
    uint32_t sector  = fat->first_data_sector + cluster * fat->sectors_per_cluster;
    uint32_t writes = (size/(fat->fs_base.sector_size_bytes * fat->sectors_per_cluster)) + 1;
    for(int i = 0; i < writes; i++){
        if(write_to_drive((uint16_t *)buffer + (i * (fat->fs_base.sector_size_bytes * fat->sectors_per_cluster)), fat->sectors_per_cluster == 0 ? 8 : fat->sectors_per_cluster, sector, fat->fs_base.drive)){
            return -1;
        }
        kprintf("sector written to: %d, sector\nByte: 0x%x (%d)", sector, sector * 512, sector * 512);
        int lcluster = cluster;
        cluster = get_next_cluster32(lcluster, fat);
        if(cluster >= 0x0fffffff){
            cluster = fat32_find_free_cluster(fat);
            set_next_cluster32(lcluster, cluster, fat);
            set_next_cluster32(cluster, -1, fat);
        }
        sector = fat->first_data_sector + cluster * fat->sectors_per_cluster;
    }
    return 0;
}
FAT_FILE *fat32_create_file(char *fname, char *root, fs_fat_t *fat){
    FAT_FILE *rdir = fat32_open_file(root, fat, MODE_READ);
    FAT_FILE *f = fat32_open_file(fname, fat, 6);
    if(f){
        kfree(rdir);
        kfree(f);
    }
    uint32_t cluster = fat->last_free_cluster == -1 ? fat32_find_free_cluster(fat) : fat->last_free_cluster;
    // if(cluster == 0){
    //     kprintf("error, exit, cluster 0");
    //     return 0;
    // }
    //realistically, this should be fine, but i should also improve it in extreme cases such as
    //dirent_count > free_memory_in_bytes/32 ((who the hell needs 1M files in a folder))
    dirent_t *dirents = kmalloc(rdir->dirent.filesize_bytes/4096 + 1, 6);
    for(int i = 0; i < rdir->dirent.filesize_bytes/4096 + 1; i++){
        uint32_t *tmp = (void *)dirents;
        tmp[i] = 0;
    }
    int i = 0;
    while(dirents[i].name[0]){
        i++;
    }
    int size = 0;
    while(fname[size] && fname[size++] == '.');
    kmemcpy(fname, dirents[i].name, size - 1);
    kmemcpy(fname + size, dirents[i].ext, kstrlen(fname + size));
    dirents[i].filesize_bytes = 0;
    set_next_cluster32(cluster, -1, fat);
    dirents[i].first_cluster = cluster & 0xffff;
    dirents[i].first_cluster_high = (cluster >> 16) & 0xffff;
    FAT_FILE *ret = kmalloc(1, 6);
    ret->dirent = dirents[i];
    ret->file_base.fs_type = FS_FAT32;
    ret->file_base.mode = MODE_WRITE;
    ret->file_base.permission = 6;
    ret->file_base.position = 0;
    ret->position_in_cluster = 0;
    ret->start_cluster = cluster;
    ret->current_cluster = cluster;
    fat32_write((FILE *)ret, rdir->dirent.filesize_bytes, dirents, fat);
    kfree(rdir);
    kfree(dirents);
    return ret;
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
        if(cluster >= 0x0FFFFFF8){
            break;
        }
        read_from_drive((uint16_t *)buffer, fat->sectors_per_cluster, fat->first_data_sector + (cluster * fat->sectors_per_cluster), fat->fs_base.drive);
        if(fatf->position_in_cluster != 0){
            kmemcpy(buffer, buffer+fatf->position_in_cluster, size);
            fatf->position_in_cluster = size;
        }
        cluster = get_next_cluster32(cluster, fat);  
        sectors /= fat->sectors_per_cluster;
    }
    fatf->position_in_cluster = size % (fat->sectors_per_cluster * fat->fs_base.sector_size_bytes);
    fatf->file_base.position += size;
    fatf->current_cluster = cluster;
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
    if(!kstrcmp(filename, "/")){
        FAT_FILE *root = kmalloc(1, 6);
        kmemcpy((char *)&fat->root_file, (char *)root, sizeof(FAT_FILE));
        return root;
    }
    char *str_to_tok = kmalloc((kstrlen(filename)/4096) + 1, 6);
    kmemcpy(filename, str_to_tok, kstrlen(filename));
    char *token = kstrtok(str_to_tok, "/");
    
    FAT_FILE *dir = &fat->root_file;
    dirent_t *dirents = kmalloc(fat->fs_base.sector_size_bytes * fat->sectors_per_cluster * 2048/4096, 6);
    dirent_t dirent;
    while(token){
        kprintf("token: %s", token);
        fat32_read((FILE *)dir, -1, (char *)dirents, fat);
        int dirsize = 0;
        //while each file has a valid name (all files must have valid names)
        //loop and inc dirsize
        while(dirents->name[0]){
            dirsize++;
        }
        dirent_t *result = search_dir(token, dirents, dirsize);
        if(!result){
            return 0;
        }
        dirent = *result;
        dir->current_cluster = result->first_cluster | (result->first_cluster_high << 16);
        // dir->dirent = *result;
        dir->start_cluster = dir->current_cluster;
        dir->file_base.position = 0;
        dir->position_in_cluster = 0;

        token = kstrtok(0, "/");
    }
    FAT_FILE *ret = kmalloc(1, 6);
    ret->dirent = dirent;
    ret->file_base.fs_type = FS_FAT32;
    ret->file_base.mode = mode;
    ret->file_base.permission = 7;
    ret->file_base.position = 0;
    ret->position_in_cluster = 0;
    ret->current_cluster = dirent.first_cluster | (dirent.first_cluster_high << 16);
    ret->start_cluster = ret->current_cluster;
    return ret;
}

fs_fat_t *fat32_register(filesystem32_t *fs, int drive, uint16_t *buffer, uint32_t partition_start_lba){
    fat32_ebpb_t *fat_boot = (fat32_ebpb_t *)buffer;
    fs_fat_t *fatfs = (fs_fat_t *)fs;
    fat32_fsinfo_t *fsinfo = kmalloc(8, 6);
    
    if(fat_boot->fat_bpb.lba_beginning == 0 && partition_start_lba){
        kprintf("Bad Start... Rewriting!\n");
        fat_boot->fat_bpb.lba_beginning = partition_start_lba;
        write_to_drive(buffer, 1, partition_start_lba, drive);
    }
    if(!read_from_drive((void *)fsinfo, 7, fat_boot->sector_FSInfo + fat_boot->fat_bpb.lba_beginning, drive)){
        kprintf("[lOSt] Error mounting FAT Partition: Error Reading from Disk\n");
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
    //Why is this one line causing me so much god damn trouble???
    //Error: hangs on assignment, won't work without being assigned
    //im going to bed...
    //THE ISSUE WAS IN A DIFFERENT FUNCTION THAT WAS LOOPING INFINITELY
    fatfs->sectors_per_cluster = fat_boot->fat_bpb.sectors_per_cluster;
    fatfs->fat_cache_size = fat_boot->fat_bpb.sectors_per_fat < 2048 ? fat_boot->fat_bpb.sectors_per_fat : 2048;
    if(fat_boot->fat_bpb.fat_c > 1){
        fatfs->fat_offset_secondary = fat_boot->fat_bpb.num_reserved_sectors + fat_size;
    }
    fatfs->root_file.start_cluster = fatfs->root_dir_cluster;
    fatfs->root_file.current_cluster = fatfs->root_dir_cluster;
    fatfs->root_file.dirent.filesize_bytes = 4096;
    kmemcpy("/", fatfs->root_file.dirent.name, 2);
    // fatfs->root_file.dirent = (dirent_t ){0};
    fatfs->root_file.file_base.fs_type = FS_FAT32;
    fatfs->root_file.file_base.mode = 6;
    fatfs->root_file.file_base.permission = 6;
    fatfs->root_file.file_base.position = 0;
    fatfs->last_free_cluster = fsinfo->last_free_cluster;
    return fatfs;
}

uint8_t fat32_create_fs(drive32_t *drive, uint32_t partition){
    
}
