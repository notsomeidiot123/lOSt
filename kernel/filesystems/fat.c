#include "../drivers/storage.h"
#include "../memory/mmanager.h"
#include "fat.h"
#include "../drivers/serial.h"

void cache_fat(fs_fat_t *fat, uint32_t start){

    uint16_t *buffer;
    buffer = fat->fat_cache == 0 ? kmalloc(fat->cached_clusters_size/8, 6) : fat->fat_cache;
    fat->cached_clusters_start = start;
    read_from_drive(buffer, fat->cached_clusters_size, start + fat->fat_offset_primary, fat->fs_base.drive);
}
void read_root_dir(fs_fat_t *fat){
    fat->root_dir_entries = kmalloc((fat->root_dir_size_sectors/8 )+ 1, 6);
    //change to allow for different sector sizes, don't wanna be wasting precious memory, do we?
    read_from_drive((uint16_t *)fat->root_dir_entries, fat->root_dir_size_sectors, fat->root_dir_sector, fat->fs_base.drive);
    
}

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
    fat->cached_clusters_size = fat->sectors_per_fat < 256 ? fat->sectors_per_fat : 256;
    // fat->free_clusters = fat16_find_free(fat);
    kprintf("FAT12/6 on drive %c: Total clusters: %d, Cluster Size (bytes): %d\nFAT Size (sectors): %d\n[      ] Caching FAT (128kb)", 'A' + drive, fat->total_clusters, fat->sectors_per_cluster * 512, fat->sectors_per_fat);
    cache_fat(fat, 0);
    fat->root_dir_size_sectors = root_dir_sectors;
    fat->root_dir_sector = fat_bpb->num_reserved_sectors + (fat_bpb->fat_c * fat_bpb->sectors_per_fat);
    read_root_dir(fat);
    return fat;
}

