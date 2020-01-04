#include "common/state.h"

using namespace bezos;

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
    for(u32 i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        terminal_buffer[i] = 0;
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
    if(c == '\n')
    {
        terminal_row++;
        terminal_column = 0;
        if(terminal_row == VGA_HEIGHT)
            terminal_row = 0;
        return;
    }
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

extern "C" byte* LOW_MEMORY;

struct e820_entry
{
    u64 addr;
    u64 len;
    u32 type;
    u32 attrib;
};

void print(const char* msg)
{
    while(*msg)
    {
        terminal_putchar(*msg);
        msg++;
    }
}

extern "C" void prot_print(const char* msg)
{
    print(msg);
}

char *convert(unsigned int num, int base) {
    static char buff[33];  

    char *ptr;    
    ptr = &buff[sizeof(buff) - 1];    
    *ptr = '\0';

    do {
        *--ptr="0123456789abcdef"[num%base];
        num /= base;
    } while(num != 0);

    return ptr;
}

void print(u32 v, u8 base)
{
    if(base == 16)
        print("0x");
    print(convert(v, base));
}

extern "C" void setup_paging(void)
{
    terminal_initialize();

    print("here\n");

    u32 eax, ebx, ecx, edx;

    asm("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x07), "c"(0));

    // check if we have pml5 support
    if(ecx & (1 << 16))
    {
        print("pml5 supported\n");
        MEMORY_LEVEL = 5;
    }
    else
    {
        print("pml4 supported\n");
        MEMORY_LEVEL = 4;
    }
    
    u32 count = 0;
    byte* offset = LOW_MEMORY;
    u32 size = 0;
    u64 usable_size = 0;
    for(;;)
    {
        e820_entry entry;
        size = *(u32*)(offset - sizeof(u32));
        
        // if the size is 0 then we're out of memory maps
        if(!size)
            break;

        entry = *(e820_entry*)(offset - 4 - size);

        if(size == 20)
            entry.attrib = 0;

        offset -= (size + 4);
        count++;

        usable_size += entry.len;

        print("\nsize = "); print(size, 10); 
        print("\nentry(addr="); print(entry.addr, 16); 
                print(",len="); print(entry.len, 16); 
                print(",type="); print(entry.type, 10);
                print(",attrib="); print(entry.attrib, 10);
                print(")\n");
    }

    print(count, 10); print(" total sections\n");
    print(usable_size / 1024 / 1024, 10); print(" usable MB of memory\n");
}