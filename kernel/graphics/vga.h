//going to keep here for backwards compatability until I refactor ALL code
extern void kputc(char c);
extern void kputs(char *s);
extern void kputd(int num);
extern void kputx(int num);
extern void kputb(int num);
extern void kprintf(char *format, ...);

extern void set_color(int color);
extern int get_color();
extern void clear_screen();

extern void textmode_print_load();
extern void disp_str(int x, int y, char *str);

extern unsigned short *get_framebuffer();
extern void set_framebuffer(unsigned short *);

extern int padding;