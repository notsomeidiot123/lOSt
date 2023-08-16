#include <stdarg.h>
#include "mmanager.h"
int kstrlen(char *s){
    int i = 0;
    while(*(s+i)){
        i++;
    }
    return i;
}
/*
    returns 0 if strings are equal.
    returns > 0 if the length or last character of s is greater than c.
    returns < 0 if the length or last character of s is less than c .
*/
int kstrcmp(char *s, char *c){
    if(kstrlen(s) != kstrlen(c)){
        return kstrlen(s)-kstrlen(c);
    }
    else{
        int i = 0;
        while(s[i]){
            if(s[i] != c[i]){
                return s[i] - c[i];
            }
            i++;
        }
    }
    return 0;
}

int kmemcmp(unsigned char *source, unsigned char *dest, int num){
    for(int i = 0; i < num; i++){
        if(source[i] != dest[i]){
            return source[i] - dest[i];
        }
    }
    return 0;
}

void kmemcpy(char *s, char *d, int bytes){
    for(int i = 0; i < bytes; i++){
        d[i] = s[i];
    }
}
void kmemset(short *d, int bytes, int val){
    for(int i = 0; i < bytes/2; i++){
        d[i] = val;
    }
}

char *ltostr(unsigned int number, int base, int padding_count){
    static char arr[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static char buffer[50];
    char *ret = &buffer[49];
    *ret = 0;
    if(base == 1){
        return 0;
    }
    do{
        *--ret = arr[number % base];
        number /= base;
        padding_count = padding_count <= 0 ? 0 : padding_count - 1;
    }while(number != 0 || padding_count);

    return ret;
}
void clearmem(void *ptr, int count){
    for(int i = 0; i < count; i++){
        ptr = 0;
    }
}
void kstrcat( char *dest, char* first, char *second){
    int pos = 0;
    while(first[pos]){
        dest[pos] = first[pos];
        pos++;
    }
    while(*second){
        dest[pos++] = *(second++);
    }
}
/*

Replaces formats inside of string {format} with arguments specified
WARNING: CALLER IS RESPONSIBLE FOR FREEING

*/
char *ksprintf(char *format, ...){
    int pages = kstrlen(format)/4096 + 1;
    char *final = kmalloc(pages, 6);
    char *str;
    int i;
    va_list arg;
    va_start(arg, format);
    for(char *fmt = format; *fmt != 0; fmt++){
        while(*fmt != '%'){
            if(*fmt == 0){
                return final;
            }
            char c[2] = {*fmt, 0};
            kstrcat(final, final, c);
            fmt++;
        }
        fmt++;
        switch(*fmt){
            case 'c' : 
                if(kstrlen(final) + 1 >= 4096 * pages){
                    char *tmp = kmalloc(++pages, 6);
                    kmemcpy(final, tmp, kstrlen(final));
                }
                i = va_arg(arg, int);
                kstrcat(final, final, (char []){i, 0});
                break;
            case 'd':
                i = va_arg(arg, int);
                if(i < 0){
                    i = -i;
                    kstrcat(final, final, "-");
                }
                if(kstrlen(final) + kstrlen(ltostr(i, 10, 0)) >= 4096 * pages){
                    char *tmp = kmalloc(++pages, 6);
                    kmemcpy(final, tmp, kstrlen(final));
                }
                kstrcat(final, final, ltostr(i, 10, 0));
                break;
            case 'x':
                i = va_arg(arg, int);
                if(kstrlen(final) + kstrlen(ltostr(i, 16, 0)) >= 4096 * pages){
                    char *tmp = kmalloc(++pages, 6);
                    kmemcpy(final, tmp, kstrlen(final));
                }
                kstrcat(final, final, ltostr(i, 16, 0));
                break;
            case 'o':
                i = va_arg(arg, int);
                if(kstrlen(final) + kstrlen(ltostr(i, 8, 0)) >= 4096 * pages){
                    char *tmp = kmalloc(++pages, 6);
                    kmemcpy(final, tmp, kstrlen(final));
                }
                kstrcat(final, final, ltostr(i, 8, 0));
                break;
            case 's':
                
                str = va_arg(arg, char *);
                if(kstrlen(final) + kstrlen(str) >= 4096 * pages){
                    char *tmp = kmalloc(++pages, 6);
                    kmemcpy(final, tmp, kstrlen(final));
                }
                kstrcat(final, final, str);
                break;
        }
    }
    return final;
}