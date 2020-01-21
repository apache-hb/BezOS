#include "common/types.h"


/* Hardware text mode color constants. */
enum vga_color {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};
 
static inline u8 vga_entry_color(enum vga_color fg, enum vga_color bg) 
{
	return fg | bg << 4;
}
 
static inline u16 vga_entry(unsigned char uc, u8 color) 
{
	return (u16) uc | (u16) color << 8;
}
 
u32 strlen(const char* str) 
{
	u32 len = 0;
	while (str[len])
		len++;
	return len;
}
 
static const u32 VGA_WIDTH = 80;
static const u32 VGA_HEIGHT = 25;
 
u32 terminal_row;
u32 terminal_column;
u8 terminal_color;
u16* terminal_buffer;
 
void terminal_initialize(void) 
{
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	terminal_buffer = (u16*) 0xB8000;
	for (u32 y = 0; y < VGA_HEIGHT; y++) {
		for (u32 x = 0; x < VGA_WIDTH; x++) {
			const u32 index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}
 
void terminal_setcolor(u8 color) 
{
	terminal_color = color;
}
 
void terminal_putentryat(char c, u8 color, u32 x, u32 y) 
{
	const u32 index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}
 
void terminal_putchar(char c) 
{
	terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
	if (++terminal_column == VGA_WIDTH) {
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT)
			terminal_row = 0;
	}
}
 
void terminal_write(const char* data, u32 size) 
{
	for (u32 i = 0; i < size; i++)
		terminal_putchar(data[i]);
}
 
void terminal_writestring(const char* data) 
{
	terminal_write(data, strlen(data));
}
 
void kernel_main(void) 
{
	/* Initialize terminal interface */
	terminal_initialize();
 
	/* Newline support is left as an exercise. */
	terminal_writestring("Hello, kernel World!\n");
}

#if 0

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_BUFFER ((u16*)0xB8000)

#if 0
static u16 vga_entry(u16 letter, u16 colour)
{
    return letter | colour << 8;
}
#endif

static u8 vga_colour(u8 fg, u8 bg)
{
    return fg | bg << 4;
}

static u8 vga_row;
static u8 vga_column;

extern void vga_init(void)
{
    terminal_initialize();

    while(1);
    vga_row = 0;
    vga_column = 0;

    for(int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        VGA_BUFFER[i] = vga_entry(' ', vga_colour(7, 0));

    while(1);
}

static void vga_put(char c)
{
    if(c == '\n')
    {
        vga_row++;
        vga_column = 0;
        return;
    }

    if(vga_column > VGA_WIDTH)
    {
        vga_row++;
        vga_column = 0;
    }

    VGA_BUFFER[vga_column * VGA_WIDTH + vga_row] = vga_entry(c, vga_colour(7, 0));
}

extern void vga_print(const char* str)
{
    while(*str)
        vga_put(*str++);
}

extern void init32(void)
{

}
#endif
