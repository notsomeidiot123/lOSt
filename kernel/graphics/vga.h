extern void kputc(char c);
extern void kputs(char *s);
extern void kputd(int num);
extern void kputx(int num);
extern void kputb(int num);
extern void kprintf(char *format, ...);

extern void set_color(int color);
extern int get_color();
extern void clear_screen();

extern int padding;