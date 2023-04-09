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

drive_t drives[16] = { 0 };

void register_drives(){
    
}