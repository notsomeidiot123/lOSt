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

void kmemcpy(short *s, short *d, int bytes){
    for(int i = 0; i < bytes/2; i++){
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