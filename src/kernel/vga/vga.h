#ifndef VGA_H
#define VGA_H

void vga_init(void);

void vga_print(const char* str);

void vga_puts(const char* str);
void vga_puti(int val, int base);

#endif
