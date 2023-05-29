unsigned char inb(unsigned short port){
    unsigned char byte = 0;
    asm volatile ("inb %1, %0" :  "=a"(byte) : "Nd"(port) );
    return byte;
}
void outb(unsigned short port, unsigned char byte){
    asm volatile ("outb %0, %1" ::  "a"(byte), "Nd"(port) );
}
void outw(unsigned short port, unsigned short data){
    asm volatile ("outw %0, %1" ::  "a"(data), "Nd"(port) );
}
unsigned short inw(unsigned short port){
    unsigned short word = 0;
    asm volatile ("inw %1, %0" :  "=a"(word) : "Nd"(port) );
    return word;
}