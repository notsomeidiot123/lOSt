#include "mmanager.h"
#include "../graphics/vga.h"

unsigned int max_address = 0;
unsigned int total_usable = 0;

page_entry_t *page_table = (page_entry_t*) 0x50000;
unsigned int page_table_size;


int check_sorted(mmap_entry_t *mmap, int mmap_entries){
    for(int i = 1; i < mmap_entries-1; i++){
        if(mmap[i].base_low < mmap[i-1].base_low + mmap[i-1].length_low){
            return i;
        }
    }
    return 0;
}

void sort_mmap(mmap_entry_t *mmap, int mmap_entries){
    if(check_sorted(mmap, mmap_entries)){
        kprintf("Error: Cannot initialize Memory!\n");
        for(;;);
    }
}

void init_memory(mmap_entry_t *mmap, int mmap_entries){
    padding = 8;
    for(int i = 0; i < mmap_entries; i++){
        unsigned int limit = mmap[i].base_low + mmap[i].length_low;
        kprintf("%d ||| Base: %x | Limit: %x | Type: %d\n", i, mmap[i].base_low, limit, mmap[i].region_type);
        
        max_address = (limit > max_address) ? limit : max_address;
        // total_usable += mmap[i].length_low * (mmap[i].region_type == 1);
        total_usable = max_address - 0x100000/4096;
    }
    
    sort_mmap(mmap, mmap_entries);
    
    padding = 0;
    int current_ent = 0;
    page_table_size = max_address/4096;
    for(int i = 0; i < (max_address / 4096); i++){
        int last_entry_max_address = (current_ent != 0) * mmap[current_ent].length_low + mmap[current_ent].base_low;
        int entry_max_address = mmap[current_ent].length_low + mmap[current_ent].base_low;
        if(i * 4096 >= entry_max_address){
            current_ent++;
        }
        if(i * 4096 < mmap[current_ent].base_low && i * 4096 > last_entry_max_address || i * 4096  < 0x00100000 || mmap[current_ent].region_type != 1){
            page_table[i] = (page_entry_t){1, 1, 0, 0, 0, 0, 0, 1};
            total_usable -= 4096;
        }
        else{
            // kprintf("Free!\n");
            page_table[i] = (page_entry_t){0, 0, 0, 0, 1, 1, 1, 0};
        }
    }
    kprintf("Total Usable Memory  : %dMB/%dMB\n", total_usable/1024/1024, max_address/1024/1024);
}

void reserve(void *address, int pages, char permissions){
    for(int i = ((long)address / 4096); i < pages; i++){
        page_table[i] = (page_entry_t){1, 1, 0, 0, permissions & 0x4, permissions & 0x2, permissions & 0x1, 1};
    }
}

int check_free(int start, int size){
    for(int i = start; i + start < start + size; i++){
        if(page_table[i].used || page_table[i].unusable || page_table[i].reserved){
            return 0;
        }
    }
    return 1;
}

void *kmalloc(int size, char permissions){
    //search map starting at 0x100000/4096
    //while reserved or used, keep searching
    //check if there is enough room
    //  if no, continue searching from the end of the just-checked space
    //  if yes, continuously allocate memory by setting .used, .linked to next if not last, .linked to last if not first, and the proper permissions
    //return the address at the start of those linked entries
    int start = 0;
    for(int i = 0x100000/4096; i < page_table_size; i++){
        if((page_table[i].used == 0) && !page_table[i].unusable && check_free(i, size)){
            int addr = i * 4096;
            for(int j = 0; j < size; j++){
                page_entry_t ent = {0};
                if(j < size - 1){
                    ent.linked_to_next = 1;
                }
                if(j > 0){
                    ent.linked_to_last = 1;
                }
                ent.used = 1;
                ent.read = permissions & 0x4;
                ent.write = permissions & 0x2;
                ent.execute = permissions & 0x1;
                page_table[i+j] = ent;
                
            }
            return (void *)(long)addr;
        }
    }
    return 0;
}

void *kfree(void *ptr){
    //calculate entry by dividing ptr by 4096
    //locate start of linked entries
    //while entry is linked to next
    //  deallocate current entry
    //  switch and read next entry
    //return null
    int start = (unsigned long)ptr/4096;
    int i = start;
    while(page_table[i].linked_to_last){
        i--;
    }
    // i--;
    // kprintf("\n%x, %x\n", i, start);
    page_entry_t last;
    do{
        last = page_table[i];
        page_table[i++] = (page_entry_t){0};
    }while(last.linked_to_next);
    return 0;
}

int get_used_pages(){
    int allocated_count = 0;
    for(int i = 0; i < page_table_size; i++){
        if(page_table[i].used && !(page_table[i].unusable || page_table[i].reserved)){
            allocated_count++;
        }
    }
    // kprintf("Debug: There are currently %d pages allocated", allocated_count);
    return allocated_count;
}

/*typedef struct page_entry{
    char used : 1;
    char unusable : 1;
    char linked_to_next:1;
    char linked_to_last:1;
    char read:1;
    char write:1;
    char execute:1;
    char reserved:1;
}page_entry_t;
*/