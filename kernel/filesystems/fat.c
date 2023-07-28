#include "../drivers/storage.h"
#include "../memory/mmanager.h"
#include "../memory/string.h"
#include "fat.h"
#include "../drivers/serial.h"

void cache_fat(fs_fat_t *fat, uint32_t start){

    uint16_t *buffer;
    buffer = fat->fat_cache == 0 ? kmalloc(fat->fat_cache_size/8, 6) : fat->fat_cache;
    fat->cached_clusters_start = start;
    read_from_drive(buffer, fat->fat_cache_size, start/128 + fat->fat_offset_primary, fat->fs_base.drive);
}
void write_cache(fs_fat_t* fat){
    write_to_drive((uint16_t *)fat->fat_cache, fat->fat_cache_size, fat->fat_offset_primary + fat->cached_clusters_start, fat->fs_base.drive);
}
void read_root_dir16(fs_fat_t *fat){
    fat->root_dir_entries = kmalloc((fat->root_dir_size_sectors/8 )+ 1, 6);
    //change to allow for different sector sizes, don't wanna be wasting precious memory, do we?
    read_from_drive((uint16_t *)fat->root_dir_entries, fat->root_dir_size_sectors, fat->root_dir_sector, fat->fs_base.drive);
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
//TODO: update ISR handler to not crash the entire fking computer
//when someone wants more ram than there is available :/
//also for every damn 512 there is in this file there's another thing i need to 
//change later :/
uint8_t *fat_read_file(uint8_t *buffer, dirent_t *dirent, uint32_t size, fs_fat_t* fat){
    uint32_t clusters = (size/fat->sectors_per_cluster * 512) + 1;
    uint32_t cluster = dirent->first_cluster | dirent->first_cluster_high << 16;
    for(int i = 0; i < clusters; i++){
        read_from_drive((uint16_t*)buffer + i * fat->sectors_per_cluster * 512, fat->sectors_per_cluster, fat->first_data_sector + cluster , fat->fs_base.drive);
        cluster = get_next_cluster(cluster, fat);
        if(cluster == -1){
            break;
        }
    }
    return buffer;
}
void fat_write_file(uint8_t *buffer, dirent_t *dirent, uint32_t size, fs_fat_t* fat){
    uint32_t clusters = (size/fat->sectors_per_cluster * 512) + 1;
    uint32_t cluster = dirent->first_cluster | dirent->first_cluster_high << 16;
    uint32_t last_cluster;
    for(int i = 0; i < clusters; i++){
        write_to_drive((uint16_t*)buffer + i * fat->sectors_per_cluster * 512, fat->sectors_per_cluster, fat->first_data_sector + cluster , fat->fs_base.drive);
        last_cluster = cluster;
        cluster = get_next_cluster(cluster, fat);
        if(cluster == -1){
            //what the hell is the special 'cluster free' 0. it's 0
            for(int i = 2; i < fat->total_clusters; i++){
                //hacky way of force-caching clusters when i need them to be, and finding out the next cluster
                int res = get_next_cluster(i, fat);
                if(!res){
                    set_next_cluster(last_cluster, res, fat);
                }
            }
        }
    }
}
// void fat16_write_root(fs_fat_t *fat){
    // fat->root_dir_entries[fat->root_dir_size_entries]
// }
FAT_FILE *fat_create_file(char *filename, fs_fat_t *fat){
    kprintf("Creating File! File:%s\n", filename);
    int last_dir_pos = 0;
    int i = 0;
    while(filename[i]){
        if(filename[i] == '/'){
            last_dir_pos = i;
        }
        i++;
    }
    filename[last_dir_pos] = 0;
    char *newfilename = filename + last_dir_pos - 1;
    dirent_t *dirent = 0;
    dirent_t *buffer = 0;
    FAT_FILE *dir = 0;
    if(filename[0]){
        dir = fat_open_file(filename, fat, MODE_READ);
        buffer = (dirent_t *)kmalloc((dir->dirent.filesize_bytes/4096) + 2, 6);

        fat_read_file((uint8_t *)buffer, &dir->dirent, dir->dirent.filesize_bytes, fat);
        int index = dir->dirent.filesize_bytes/sizeof(dirent_t);
        dirent = &(buffer[index]);
    }
    else if(fat->fs_base.type == FS_FAT32){
        //open file and read clusters
        kprintf("fat32");
    }
    else{
        //create a new file and shove it into the root folder ig
        kprintf("else\n");
    }
    kmemcpy((short *)newfilename, (short *)dirent->name, 8);
    kmemcpy((short *)newfilename + 8, (short *)dirent->ext, 3);
    dirent->filesize_bytes = 0;
    for(int i = 2; i < fat->total_clusters; i++){
    //hacky way of force-caching clusters when i need them to be, and finding out the next cluster
        int res = get_next_cluster(i, fat);
        if(!res){
            set_next_cluster(res, res, fat);
            dirent->first_cluster = res & 0xff;
            dirent->first_cluster_high = (res >> 16) & 0xff;
        }
    }
    fat_write_file((uint8_t *)buffer, &dir->dirent, dir->dirent.filesize_bytes + 32, fat);
    FAT_FILE *file = kmalloc(1, 6);
    file->file_base.fs_type = FS_FAT;
    file->file_base.mode = MODE_READ | MODE_WRITE;
    file->start_cluster = dirent->first_cluster | dirent->first_cluster_high << 16;
    file->current_cluster = file->start_cluster;
    file->dirent = *dirent;
    file->file_base.position = 0;
    return file;
    return 0;
}
//warning: this function allocates memory
FAT_FILE *fat_open_file (char *filename, fs_fat_t *fat, uint8_t mode){
    //validation should have been done in the calling function
    int splitcount = 0;
    int stream_pos = 2;
    char *splitname = (char *)kmalloc(1, 6);
    int i = 0;
    char *filename_base = filename;
    while(filename[i]){
        if(filename[i] == '/'){
            splitcount++;
            filename[i] = 0;
        }
        i++;
    }
    uint16_t* dir = kmalloc(((fat->root_dir_size_entries * 32)/4096) + 1, 6);
    kmemcpy((short *)fat->root_dir_entries, (short*)dir, fat->root_dir_size_entries * 32);
    int dirsize = fat->root_dir_size_entries;
    dirent_t *dirent;
    while(*filename){
        filename++;
    }
    filename++;
    for(int i = 0; i < splitcount; i++){
        dirent = search_dir(filename, (dirent_t *)dir, dirsize);
        if(!dirent){
            kfree(dir);
            break;
        }
        //else, resize dir to appropriate size, follow cluster chain and 
        //read new directory from disk
        //increment current filename past current null terminator;
        //if last split, exit < done automatically ig
        
        uint16_t *tmpdir = kmalloc((dirent->filesize_bytes/4096) + 1, 6);
        dirsize = dirent->filesize_bytes/32;
        //of course, cast it from a uint16_t * to a uint8_t* to a uint16_t*, what's wrong
        //with that???
        fat_read_file((uint8_t*)tmpdir, dirent, dirent->filesize_bytes, fat);
        kfree(dir);
        while(*filename){
            filename++;
        }
        filename++;
        dir = tmpdir;
    }
    kprintf("filename: %s", filename);
    dirent = search_dir(filename, (dirent_t*)dir, dirsize);
    if(!dirent){
        kfree(dir);
        if(mode & MODE_WRITE){
            return fat_create_file(filename, fat);
        }
        
        return 0;
    }
    FAT_FILE *file = kmalloc(1, 6);
    file->file_base.fs_type = FS_FAT;
    file->file_base.mode = mode;
    file->start_cluster = dirent->first_cluster | dirent->first_cluster_high << 16;
    file->current_cluster = file->start_cluster;
    file->dirent = *dirent;
    file->file_base.position = 0;
    kprintf("reached end");
    return file;
    //repeat search with last file, if it is nonexistent, and the mode is
    //"write", create a file, else, either return null or return null, 
    //may switch to handling in vfs
}





//we're gonna ignore these
void fat_read(FILE *file, int size, char *buffer);
int fat_write(FILE *file, int size, char *buffer);

//ugly
fs_fat_t *register_fat16(filesystem32_t *fs, int drive, uint16_t *buffer){

    fat_bpb_t* fat_bpb = (fat_bpb_t *)buffer;
    if(fat_bpb->bytes_per_sector == 0 || fat_bpb->sectors_per_cluster == 0){
        return 0;
    }
    uint32_t fat_size = fat_bpb->sectors_per_fat;
    uint32_t root_dir_sectors = ((fat_bpb->root_dir_entries * 32 ) + (fat_bpb->bytes_per_sector - 1)) / fat_bpb->bytes_per_sector == 0;
    uint32_t data_sectors = (fat_bpb->sectors_in_vol == 0 ? fat_bpb->large_sector_count : fat_bpb->sectors_in_vol) - (fat_bpb->num_reserved_sectors + fat_bpb->fat_c * fat_size + root_dir_sectors);
    uint32_t total_clusters = data_sectors/fat_bpb->sectors_per_cluster;
    fs_fat_t *fat = (fs_fat_t*)fs;
    fat->fs_base.drive = drive;
    fat->fs_base.flags = (struct fs_flags){0};
    fat->fs_base.size_low = fat_bpb->sectors_in_vol == 0 ? fat_bpb->large_sector_count : fat_bpb->sectors_in_vol;
    fat->first_data_sector = fat_bpb->num_reserved_sectors + (fat_bpb->fat_c * fat_bpb->sectors_per_fat);
    fat->sectors_per_cluster = fat_bpb->sectors_per_cluster;
    fat->sectors_per_fat = fat_bpb->sectors_per_fat;
    fat->fat_offset_primary = fat_bpb->num_reserved_sectors;
    fat->fat_offset_secondary = fat_bpb->num_reserved_sectors + fat_bpb->sectors_per_fat;
    fat->total_clusters = total_clusters;
    fat->fat_cache_size = fat->sectors_per_fat < 256 ? fat->sectors_per_fat : 256;
    fat->cached_clusters_size = fat->fat_cache_size * 128;
    // fat->free_clusters = fat16_find_free(fat);
    kprintf("FAT12/6 on drive %c: Total clusters: %d, Cluster Size (bytes): %d\nFAT Size (sectors): %d\n[      ] Caching FAT (128kb)", 'A' + drive, fat->total_clusters, fat->sectors_per_cluster * 512, fat->sectors_per_fat);
    cache_fat(fat, 0);
    kprintf("\r[ DONE ]\n");
    fat->root_dir_size_sectors = root_dir_sectors;
    fat->root_dir_sector = fat_bpb->num_reserved_sectors + (fat_bpb->fat_c * fat_bpb->sectors_per_fat);
    read_root_dir16(fat);
    return fat;
}

