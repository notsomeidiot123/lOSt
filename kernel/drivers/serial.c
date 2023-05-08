#include "../memory/mmanager.h"
#include "../libs/types.h"
#include "../cpu/io.h"
#include "../memory/string.h"
#include <stdarg.h>


struct com_ports{
    uint16_t com[4];
    struct com_desc_s{
        void *listeners[4];
        char listener_data_buffer[4][32];
        int listner_data_index[4];
        uint8_t virtual:1;
    }com_desc[4];
}ports;

enum Parity{
    NONE,
    ODD = 0b001,
    EVEN = 0b011,
    MARK = 0b101,
    SPACE = 0b111,
};

typedef struct int_en_rs{
    uint8_t data_available:1;
    uint8_t trans_empt:1;
    uint8_t break_error:1;
    uint8_t stat_change:1;
    uint8_t unused: 4;
}int_en_t;

typedef struct modem_control_s{
    uint8_t data_ready:1;
    uint8_t req_send:1;
    uint8_t out1:1;
    uint8_t out2:1;
    uint8_t loopback:1;
    uint8_t unused:3;
}modem_ctrl_t;

typedef struct line_control_s{
    uint8_t char_len:2;
    uint8_t stop_bits:1;
    uint8_t parity:3;
    uint8_t unknown:1;
    uint8_t dlab:1;
}line_ctrl_t;

typedef struct line_status_s{
    uint8_t data_ready:1;
    uint8_t overrun_e:1;
    uint8_t parity_e:1;
    uint8_t framing_e:1;
    uint8_t break_ind:1;
    uint8_t trans_holding_empt:1;
    uint8_t trans_empt:1;
    uint8_t impending_e:1;
}line_stat_t;

typedef struct modem_status_s{
    uint8_t cts_changed:1;
    uint8_t dsr_changed:1;
    uint8_t ri_pulled_high:1;
    uint8_t dcd_changed:1;
    uint8_t ncts:1;
    uint8_t ndsr:1;
    uint8_t nri:1;
    uint8_t ndcd:1;
}modem_stat_t;

typedef struct fifo_control_s{
    uint8_t fifo_en:1;
    uint8_t clear_recv_fifo:1;
    uint8_t clear_trans_fifo:1;
    uint8_t dma_mode:1;
    uint8_t undef:2;
    uint8_t trigger_level:2;
}fifo_ctrl_t;

#define DATA + 0
#define INTEN + 1
#define DIVISOR_LSB + 0
#define DIVISOR_MSB + 1
#define INTID + 2
#define FIFO + 2
#define LINECTRL + 3
#define MODEMCTRL + 4
#define LINESTAT + 5
#define MODEMSTAT + 6
#define SCRATCH + 7

char serial_init(){
    //for clarity when coding the actual stuff;
    int failed_ports = 0;
    for(int i = 0; i < 4; i++){
        ports.com[i] = bda->com_ports[i];
        uint16_t port = ports.com[i];
        outb(port INTEN, 0);
        outb(port LINECTRL, 0x80);//set msb of line_ctrl_t
        outb(port DIVISOR_LSB, 3);
        outb(port DIVISOR_MSB, 0);
        outb(port LINECTRL, 3); //set to 8 bits, no parity, 1 stop
        outb(port FIFO, 0b11000111); //enable and clear FIFO, set buffer to 14 bytes
        outb(port MODEMCTRL, 0xb);
        outb(port MODEMCTRL, 0x1e);
        outb(port DATA, 0x41);
        
        ports.com_desc[i].virtual = 0;
        for(int j = 0; j < 4; j++){
            ports.com_desc[i].listeners[j] = 0;
        }
        if(inb(port DATA) != 0x41){
            failed_ports |= 1 << i;
            ports.com_desc[i].virtual = 1;
        }
        else{
            outb(port MODEMCTRL, 0xe);
        }
    }
    return failed_ports;
}

void serial_write(int port, uint8_t c){
    for(int i = 0; i < 4; i++){
        if(ports.com_desc[port].listeners[i] == 0){
            continue;
        }
        ports.com_desc[port].listener_data_buffer[i][ports.com_desc[port].listner_data_index[i]] =
                ((char (*)(uint8_t))ports.com_desc[port].listeners[i])(c);
    }    
    if(!ports.com_desc[port].virtual){
        outb(ports.com[port] DATA, c);
    }
}
uint8_t serial_read(int port){
    if(!ports.com_desc[port].virtual){
        return inb(ports.com[port] DATA);
    }
    return 0;
}

int base_color;
int buffer_index = 0;
int xpos = 0;
int ypos = 0;
int padding = 1;

//moved from graphics/vga.c
void clear_screen(){
    // kmemset((short* )vga_buffer, video_mode.xres * video_mode.yres*2, base_color << 8);
    buffer_index = 0;
}

void kputc(char c){
    
    serial_write(0, c);
    if(c == '\b'){
        serial_write(0, ' ');
        serial_write(0, '\b');
    }
    if(c == '\n'){
        serial_write(0, '\r');
    }
    
}

// void kputc(char c){
//     char col = (char)base_color;
//     switch (c) {
//         case '\b':
//             if(buffer_index <= 0){
//                 buffer_index = 1;
//             }
//             // vga_buffer[--buffer_index] = base_color << 8 | 0;
//             serial_write(0, '\b');
//             break;
//         case '\t':
//             for(int i = 0; i < 4; i++){
//                 kputc(' ');
//             }
//             break;
//         case '\n':
//             // buffer_index += 80 - (buffer_index % 80);
//             break;
//         case '\r':
//             buffer_index -= buffer_index % 80;
//             break;
//         case '\e':
//             // clear_screen();
//             buffer_index = 0;
//             break;
//         case '\a':
//             //bell
//             break;
//         default:
//             // vga_buffer[buffer_index++] = base_color << 8 | c;
//             serial_write(0, c);
//             break;
//     }
//     // if(buffer_index >= 80*25){
//     //     kmemcpy((short *)vga_buffer+80, (short *)vga_buffer, 80*25*2 - 160);
//     //     kmemset((short *)vga_buffer+(80*24), 160, base_color);
//     //     buffer_index = 80*24;
//     // }
//     //implement scrolling with memcpy :) :finsihed:
// }

void kputs(char *s){
    while(*s){
        kputc(*s);
        s++;
    }
}

void kputd(int num){
    int i = num < 0 ? -num : num;
    kputs(ltostr(i, 10, padding));
}

void kputx(unsigned int num){
    kputs(ltostr(num, 16, padding));
}

void kputb(int num){
    kputs(ltostr(num, 2, padding));
}

void kprintf(char *format, ...){
    char *str;
    int i;
    va_list arg;
    va_start(arg, format);
    for(char *fmt = format; *fmt != 0; fmt++){
        while(*fmt != '%'){
            if(*fmt == 0){
                return;
            }
            kputc(*fmt);
            fmt++;
        }
        fmt++;
        switch(*fmt){
            case 'c' : 
                i = va_arg(arg, int);
                kputc(i);
                break;
            case 'd':
                i = va_arg(arg, int);
                if(i < 0){
                    i = -i;
                    kputc('-');
                }
                kputd(i);
                break;
            case 'x':
                i = va_arg(arg, int);
                kputx(i);
                break;
            case 'o':
                i = va_arg(arg, int);
                kputs(ltostr(i, 8, 0));
                break;
            case 's':
                str = va_arg(arg, char *);
                kputs(str);
                break;

        }
    }
}


void set_color(int color){
    base_color = color;
}
int get_color(){
    return base_color;
}