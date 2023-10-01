#include "kb.h"

kb_handler g_listener = 0;

void kb_listener(uint8_t key){
    if(!g_listener){
        return;
    }
    g_listener(key);
}

int register_kb_listener(void (*listener)(uint8_t)){
    if(!listener){
        return KB_ERR_INVALID_FP;
    }
    else if(g_listener){
        //modify to ask permission to replace the listener
        return KB_ERR_DUAL_REG;
    }
    g_listener = listener;
    return 0;
}
void request_register_driver(void (*driver_listener)(kb_handler)){
    if(driver_listener){
        driver_listener(kb_listener);
    }
}
