#pragma once
#include "../libs/types.h"
typedef struct mmap_entry{
    unsigned int base_high;
    unsigned int base_low;
    unsigned int length_high;
    unsigned int length_low;
    unsigned int attributes;
    unsigned int region_type;
    
}mmap_entry_t;

typedef struct page_entry{
    uint8_t pid;
    char used : 1;
    char unusable : 1;
    char linked_to_next:1;
    char linked_to_last:1;
    char read:1;
    char write:1;
    char execute:1;
    char reserved:1;
}page_entry_t;

#define MEMORY_PERM_READ 0x4
#define MEMORY_PERM_WRTE 0x2
#define MEMORY_PERM_EXEC 0x1

enum BDA_VIDEO_MODE{
    BDA_VM_EGA,
    BDA_VM_c40x25,
    BDA_VM_c80x25,
    BDA_VM_m80x25
};

#define BDA 0x400

typedef struct BDA_S{
    uint16_t com_ports[4];
    uint16_t lpt_ports[4];
    struct eq_word_s{
        uint16_t boot_floppy:1;
        uint16_t fpu_coproc:1;
        uint16_t ps_2_mouse:1;
        uint16_t res:1;
        uint16_t video_mode:2;
        uint16_t installed_fd:1;
        uint16_t res0:1;
        uint16_t serial_ports_c:3;
        uint16_t res1:2;
        uint16_t parallel_ports_c:2;
    }hw_flags;
    uint8_t iflag;
    uint16_t memsize_kb;
    uint16_t ecodes;
    struct kb_flags0_s{
        uint8_t rshift:1;
        uint8_t lshift:1;
        uint8_t ctrl:1;
        uint8_t alt:1;
        uint8_t scllk:1;
        uint8_t numlok:1;
        uint8_t capslk:1;
        uint8_t insert:1;
    }kb_flags0;
    struct kb_flags1_s{
        uint8_t ralt_pressed:1;
        uint8_t lalt_pressed:1;
        uint8_t sysreg_pressed:1;
        uint8_t pause_key:1;
        uint8_t scllk_pressed:1;
        uint8_t numlok_pressed:1;
        uint8_t capslk_pressed:1;
        uint8_t insert_pressed:1;
    }kb_flags1;
    uint8_t altnumpad;
    uint16_t next_kb_char;
    uint16_t last_kb_char;
    char kb_buffer[32];
    struct floppy_status_s{
        uint8_t fd0_calibrated:1;
        uint8_t fd1_calibrated:1;
        uint8_t fd2_calibraded:1;
        uint8_t fd3_calibrated:1;
        uint8_t res0:4;
        uint8_t mot0:1;
        uint8_t mot1:1;
        uint8_t mot2:1;
        uint8_t mot3:1;
        uint8_t selected_drive:2;
        uint8_t res1:1;
        uint8_t last_op:1;
    }floppy_status;
    uint8_t floppy_mot_timeout;
    struct hd_fd_statreg_0_s{
        uint8_t drive_sel:2;
        uint8_t head_state:1;
        uint8_t drive_ready:1;
        uint8_t drive_fault:1;
        uint8_t seek:1;
        uint8_t int_code:2;
    }drives_stat;
    struct fdc_stat_s{
        uint8_t missing_addr:1;
        uint8_t write_protected:1;
        uint8_t sector_not_found:1;
        uint8_t unused0:1;
        uint8_t dma_overrun:1;
        uint8_t crc_during_read:1;
        uint8_t unused1:1;
        uint8_t lba_overflow:1;
    }fdc_stat[2];
    uint8_t fdc_cyl;
    uint8_t fdc_head;
    uint8_t fdc_sec;
    uint8_t last_written_c;
    uint16_t video_mode; //might need to change this to single byte?
    uint16_t cols_per_row;
    uint16_t video_mode_sz;
    uint16_t video_page_start;
    uint16_t vpage_cursors[8];
    uint16_t cursor_shape;
    uint8_t vpage_index;
    uint16_t display_ioport;
    uint8_t display_mode_reg;
    uint8_t color_palette;
    uint16_t adapter_rom_offset;
    uint16_t adapter_rob_seg;
    uint8_t last_int;
    uint32_t int1ah_counter;
    uint8_t dayflag;
    uint8_t kb_ctrl_brk;
    uint16_t soft_reset;
    uint8_t last_hd_op_stat;
    uint8_t hd_c;
    uint8_t hd_control;
    uint8_t offset_hd_port;
    uint8_t parallel_ports_timeout[4];
    uint8_t serial_ports_timeout[4];
    char res0;
    uint16_t kb_buffer_addr;
    uint16_t end_kb_buffer;
    uint8_t vrows_c;
    uint16_t scanlines_per_char;
    struct video_display_adapter_os{
        uint8_t cursor:1;
        uint8_t monitor_color:1;
        uint8_t res0:1;
        uint8_t video_subsystem:1;
        uint8_t memory_c:3;
        uint8_t clear_on_set:1;
    }video_display_opts;
    struct video_flags_s{
        uint8_t vga_active:1;
        uint8_t grayscale_summing:1;
        uint8_t color_type:1;
        uint8_t palette_loading:1;
        uint8_t mode_lsb:1;
        uint8_t reserved:1;
        uint8_t display_switch:1;
        uint8_t mode_msb:1;
    }vga_video_flags[2];
    uint8_t fd_config_data;
    uint8_t hd_status;
    uint8_t hd_error;
    uint8_t hd_task_complete;
    uint8_t fdd_info;
    uint8_t diskettes_state[2];
    uint8_t diskettes_op_start[2];
    uint8_t diskette_cyl[2];
    uint8_t kb_state_ext[2];
    uint32_t userwait_ptr;
    uint32_t userwait_c;
    uint8_t userwait_f;
    uint8_t lab_bytes[7];
    uint32_t vpcb_ptr;
    uint8_t res2[68];
    uint8_t intra_app_comms;
}__attribute__((packed)) bda_t;

extern void init_memory(mmap_entry_t *mmap, int mmap_entries);
extern void *kmalloc(int size, char permissions);
extern void *kfree(void *ptr);
extern int get_used_pages();
extern bda_t *bda;