#include "../memory/mmanager.h"
#include "../libs/types.h"
#include "../cpu/io.h"
struct com_ports{
    uint16_t com[4];
    struct com_desc_s{
        uint8_t virtual_tty:1;
        // uint8_t 
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
    for(int i = 0; i < 4; i++){
        ports.com[i] = bda->com_ports[i];
        uint16_t port = ports.com[i];
        outb(port INTEN, 0);
        outb(port LINECTRL, 0x80);//set msb of line_ctrl_t
        outb(port DIVISOR_LSB, 3);
        outb(port DIVISOR_MSB, 0);
        outb(port LINECTRL, 3); //set to 8 bits, no parity, 1 stop
        outb(port FIFO, 0b11000111); //enable and cleare FIFO, set buffer to 14 bytes
        outb(port MODEMCTRL, 0xb);
        outb(port MODEMCTRL, 0x1e);
        outb(port, 0x41);
        if(inb(port) != 0x41){
            return i + 1;
        }
    }
    return 0;
    
}