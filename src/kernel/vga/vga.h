#ifndef VGA_VGA_H
#define VGA_VGA_H 1

void vga_init();

void vga_print(const char* str);

void vga_puts(const char* str);
void vga_puti(int val, int base);

#endif // VGA_VGA_H
