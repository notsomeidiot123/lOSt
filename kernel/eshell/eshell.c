#include "../memory/mmanager.h"
#include "../memory/string.h"
#include "../drivers/storage.h"
#include "../drivers/serial.h"
#include "../graphics/vga.h"
#include "../drivers/timer.h"
#include "../cpu/io.h"
#include "../drivers/ps2.h"
#include "../libs/defs.h"
#include <stdint.h>

char *current_command;
char *last_command;



struct{
    uint8_t color;
    char *current_directory;
    uint8_t exit;
    uint32_t driver_key;
    struct user{
        struct uinfo{
            uint8_t valid :1;
            uint8_t exec:1;
            uint8_t edit:1;
            uint8_t delete:1;
            uint8_t kernel:1;
        }uinfo;
        uint8_t user_id;
        uint32_t password_hash;
        char username[32];
    }users[32];
    uint8_t current_user;
}eshell_settings = {0};
uint8_t lock_last_line = 0;

int x = 1;
int y = 0;
int minx = 1;
int miny = 0;

uint8_t active_proc = 0;

void tmp_putc(char c){
    char str[2] = {c, 0};
    disp_str(x, y, str);
}

void write_screen_char(char c){
    if(c == '\n'){
        disp_str(x, y, " ");
        x = 1;
        y++;
    }
    else if(c == '\r'){
        x = 1;
    }
    else if(c== '\b'){
        // disp_str(x--, y, " ");
        x--;
        if(x <= minx) x = minx;
        if(y <= miny) y = miny;
        if(x < 1){
            if(!lock_last_line){
                x = 1;
            }
            else{
                x = 80;
                y--;
            }
        }
    }
    else{
        char str_to_disp[2] = {c, 0};
        disp_str(x++, y, str_to_disp);
    }
    if(x > 80){
        x = 1;
        y++;
    }
    if(y >= 24){
        void *framebuffer = get_framebuffer();
        kmemcpy(framebuffer + 160, framebuffer, 80*24 * 2);
        
        y = 23;
        x = 1;
    }
    
}

void write_screen(char *str){
    while(*str){
        write_screen_char(*(str++));
    }
}

void exec_command(char *command){
    if(command[0] == 0){
        return;
    }
    if(!kstrcmp(command, "exit")){
        write_screen("exiting...\n");
        eshell_settings.exit = 1;
    }
    else if(!kstrcmp(command, "clear")){
        kmemset((void *)get_framebuffer(), 80*24*2, 0);
        x = 1;
        y = 0;
    }
    else if(!kstrcmp(command, "disks")){
        write_screen("Showing mounted Disks\n");
        int last = -1;
        for(int i = 0; i < 26; i++){
            if(i == last){
                continue;
            }
            drive32_t *drive = get_drive(i);
            last = i;
            if(drive == 0){
                continue;
            }
            char *string = ksprintf("%s%c:\n\tType: %s\n\tSize: %dMB\n\tBytes Per Sector: %d\n", disk_types_ids[drive->type], 'a' + i, drive_type_strs[drive->type],(drive->size_low * drive->bytes_per_sector)/1024/1024, drive->bytes_per_sector);
            write_screen(string);
            kfree(string);
        }
    }
    else if(!kstrcmp(command, "cd ../")){
        int last = 0;
        int i = 1;
        if(kstrlen(eshell_settings.current_directory) < 3){
            return;
        }
        while(eshell_settings.current_directory[i]){
            if(eshell_settings.current_directory[i] == '/'){
                last = i;
            }
            i++;
        }
        eshell_settings.current_directory[last] = 0;
    }
    else if(!kstrcmp(command, "user")){
        if(!eshell_settings.users[eshell_settings.current_user].uinfo.valid){
            write_screen("lOSt\n");
        }
        else{
            write_screen(eshell_settings.users[eshell_settings.current_user].username);

        }
    }
    else if(!kstrcmp(command, "osinfo")){
        write_screen("Kernel: ");
        write_screen(OS_VERSION);
        write_screen("\n");
        write_screen("Shell: ");
        write_screen(SHELL_VERSION);
        write_screen("\n");
    }
    else if(!kstrcmp(command, "login")){
        write_screen("WIP\n");
    }
    else{
        write_screen("Command not found: ");
        write_screen(command);
        write_screen("\n");
    }
}

void getch(char c){
    if(current_command != 0){
        write_screen_char(c);
        if(c == '\n'){
            // write_screen_char(c);
            exec_command(current_command);
            if(!active_proc && eshell_settings.exit == 0){
                write_screen(eshell_settings.current_directory);
                write_screen("/>");
                minx = x;
                miny = y;
            }
            if(last_command != 0) kfree(last_command);
            last_command = current_command;
            current_command = kmalloc(1, 6);
            for(int i = 0; i < 4096; i++){
                current_command[i] = 0;
            }
            
        }
        else if(c < ' '){
            if(c == '\b'){
                current_command[kstrlen(current_command) - 1] = 0;
            }
            return;
        }
        else{
            char to_cat[2] = {c, 0};
            kstrcat(current_command, current_command, to_cat);
        }
        
    }
}
void eshell(){
    write_screen("Warning: Execution of some programs may be limited, especially those which require command-line arguments\n");
    write_screen("[      ] Initializing Shell");
    //disable tm cursor
    outb(0x3d4, 0xa);
    outb(0x3d5, 0x20);
    toggle_auto_return();
    eshell_settings.current_directory = kmalloc(1, 6);
    kmemcpy((void *)"A:",(void *) eshell_settings.current_directory, 4);
    current_command = kmalloc(1, 6);
    eshell_settings.driver_key = register_serial_listener((void*)getch, 0, 0);
    write_screen("\r[ DONE ]\n");
    write_screen("Use the command \'help\' to see builtin commands!\n");
    write_screen(eshell_settings.current_directory);
    write_screen(">");
    minx = x;
    miny = y;
    while(!eshell_settings.exit){
        tmp_putc(' ');
        wait_secs(1);
        while(!wait_secs(0));
        tmp_putc('_');
        wait_secs(1);
        while(!wait_secs(0));
    }
    toggle_auto_return();

    deregister_serial_listener(0, eshell_settings.driver_key);
}