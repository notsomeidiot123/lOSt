#include "../drivers/storage.h"
#include "../memory/mmanager.h"
#include "../memory/string.h"
#include "fat.h"
#include "../drivers/serial.h"

void cache_fat(fs_fat_t *fat, uint32_t start){
    //woo that took me a while to re-read
    //remember to replace 128 with bytes_per_sector/4 (four bytes per FAT entry)
    uint16_t *buffer;
    buffer = fat->fat_cache == 0 ? kmalloc(fat->fat_cache_size/8, 6) : fat->fat_cache;
    fat->cached_clusters_start = start;
    read_from_drive(buffer, fat->fat_cache_size, start/128 + fat->fat_offset_primary, fat->fs_base.drive);
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

uint32_t get_next_cluster(uint32_t cluster, fs_fat_t *fat){
    //i feel like this is broken... we'll see later
    if(cluster < fat->cached_clusters_start/128 || cluster > fat->cached_clusters_start/128 + fat->cached_clusters_size){
        cache_fat(fat, cluster);
    }
    return fat->fat_cache[cluster - fat->cached_clusters_start/128];
}
void set_next_cluster(uint32_t cluster, uint32_t next, fs_fat_t *fat){
    if(cluster < fat->cached_clusters_start/128 || cluster > fat->cached_clusters_start/128 + fat->cached_clusters_size){
        write_cache(fat);
        cache_fat(fat, cluster);
    }
    if(cluster == next){
        next = -1;
    }
    fat->fat_cache[cluster - fat->cached_clusters_start/128] = next;
}
