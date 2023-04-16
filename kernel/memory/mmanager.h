typedef struct mmap_entry{
    unsigned int base_high;
    unsigned int base_low;
    unsigned int length_high;
    unsigned int length_low;
    unsigned int attributes;
    unsigned int region_type;
    
}mmap_entry_t;

typedef struct page_entry{
    char used : 1;
    char unusable : 1;
    char linked_to_next:1;
    char linked_to_last:1;
    char read:1;
    char write:1;
    char execute:1;
    char reserved:1;
}page_entry_t;

extern void init_memory(mmap_entry_t *mmap, int mmap_entries);
extern void *kmalloc(int size, char permissions);
extern void *kfree(void *ptr);
extern int get_used_pages();