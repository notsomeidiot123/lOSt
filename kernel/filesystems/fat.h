#pragma once
#include <stdint.h>
#include "../drivers/storage.h"
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

#define is_lfn(file) file->attributes.read_only && file->attributes.hidden && file->attributes.system && file->attributes.vol_id

typedef struct fat_file_s{
    char name[8];
    char ext[3];
    struct fat_file_attributes_s{
        uint8_t read_only:1;
        uint8_t hidden:1;
        uint8_t system:1;
        uint8_t vol_id:1;
        uint8_t is_dir:1;
        uint8_t archive:1;
    }attributes;
    uint8_t reserved;
    uint8_t creation_time_tenths;
    struct fat_time_s{
        uint16_t hour:5;
        uint16_t minutes:6;
        uint16_t seconds:5;
    }creation_time;
    struct fat_date_s{
        uint16_t year:7;
        uint16_t month:4;
        uint16_t day:5;
    }creation_date;
    struct fat_date_s last_accessed;
    uint16_t first_cluster_high;
    struct fat_time_s last_mod_time;
    struct fat_date_s last_mod_date;
    uint16_t first_cluster;
    uint32_t filesize_bytes;
    
}fat_file_t;

typedef struct fs_fat_s{
    filesystem32_t fs_base;
    //Offset from start of partition, not start of drive
    uint32_t fat_offset_primary;
    //Offset from start of partition, not start of drive
    uint32_t fat_offset_secondary;
    uint16_t sectors_per_fat;
    uint32_t total_clusters;
    uint32_t sectors_per_cluster;
    uint32_t first_data_sector;
    uint32_t free_clusters;

    uint32_t cached_clusters_start;
    uint32_t cached_clusters_size;
    uint16_t *fat_cache;
    uint32_t last_free_cluster;
    uint32_t fat_cache_size;
    
    uint32_t root_dir_sector;
    fat_file_t *root_dir_entries;
    uint32_t root_dir_size_sectors;
    uint32_t root_dir_size_entries;
}fs_fat_t;

typedef struct fat_file_type_s {
    FILE file_base;
    uint32_t start_cluster;
    uint32_t current_cluster;
}FAT_FILE;

typedef fat_file_t dirent_t;

FAT_FILE *fat_open_file(char *filename, fs_fat_t* fat);
void fat_read(FILE *file, int size, char *buffer);
int fat_write(FILE *file, int size, char *buffer);
fs_fat_t *register_fat16(filesystem32_t *fs, int drive, uint16_t *buffer);