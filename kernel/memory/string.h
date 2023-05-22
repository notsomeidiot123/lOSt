extern int kstrlen(char *s);
extern int kstrcmp(char *s, char *d);
extern void kmemset(short *d, int bytes, int val);
extern void kmemcpy(short *s, short *d, int bytes);
extern char *ltostr(int number, int base, int padding);
extern void clearmem(void *ptr, int count);