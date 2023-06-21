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
}fs_fat_t;
fs_fat_t *register_fat16(filesystem32_t *fs, int drive, uint16_t *buffer);