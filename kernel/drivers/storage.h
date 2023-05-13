#pragma once

enum Drive_Types{
    DRIVE_NULL,
    DRIVE_VIRT,
    DRIVE_FLOPPY,
    DRIVE_PATA,
    DRIVE_ATAPI,
    DRIVE_SATA,
    DRIVE_USB,
};

typedef struct drive{
    char number;
    char type;
    struct drive_flags{
        char read_only:1;
        char removable:1;
        char hidden:1;
        char contains_pt:1;
        char reserved:4;
    }flags;
    void *extended_struct;
}drive_t;

extern int register_drive(drive_t drive_to_register);