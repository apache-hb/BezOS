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

template<typename T>
inline T page_align(T val) 
{ 
    return (T)((reinterpret_cast<u32>(val) + 0x1000 - 1) & -0x1000); 
}

template<typename T>
void wipe_page(T* page)
{
    for(int i = 0; i < 512; i++)
        page[i] = (T)0;
}

struct segment
{
    segment() {}

    u32 limit;
};

struct gdt_entry
{
    u32 limit_low:16;
    u32 base_low:16;
    u8 type:4;
    u8 cls:1;
    u8 dpl:2;
    u8 present:1;
    u32 limit_high:4;
    u8 avail:1;
    u8 long_mode:1;
    u8 op_size:1;
    u8 granularity:1;
    u32 base_high:8;
}
__attribute__((packed));

struct gdt_ptr
{
    u16 size;
    u32 offset;
}
__attribute__((packed));

u64 last_page;

extern "C" void get_last_page()
{
    return last_page;
}

extern "C" void* setup_paging(void)
{
    terminal_initialize();

    u32 eax, ebx, ecx, edx;

    asm("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x07), "c"(0));

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

    // align the low memory to the page
    LOW_MEMORY = page_align(LOW_MEMORY);

    // check if we have pml5 support
    if(ecx & (1 << 16))
    {
        print("pml5 supported\n");
        MEMORY_LEVEL = 5;
        page5 = (PML5)(LOW_MEMORY);
        LOW_MEMORY += 0x1000;
    }
    else
    {
        print("pml4 supported\n");
        MEMORY_LEVEL = 4;

        // find where to put the tables
        auto p4 = (PML4)(LOW_MEMORY += 0x1000);
        page4 = p4;
        wipe_page(p4);
    }

    auto p3 = (PML3)(LOW_MEMORY += 0x1000);
    auto p2 = (PML2)(LOW_MEMORY += 0x1000);
    auto pt = (PT)(LOW_MEMORY += 0x1000);
    
    // then clear memory because undefined behaviour is spooky
    wipe_page(p3);
    wipe_page(p2);
    wipe_page(pt);

    int page_index = 0;

    if(MEMORY_LEVEL == 5)
        pt[page_index++] = (u64)page5;

    pt[page_index++] = (u64)page4;
    pt[page_index++] = (u64)p3;
    pt[page_index++] = (u64)p2;
    pt[page_index++] = (u64)pt;
    pt[page_index++] = (u64)(LOW_MEMORY += 0x1000);
    last_page = (u64)LOW_MEMORY;

    return top_page;
}

// at this point paging is enabled
// lets be careful here
extern "C" void setup_gdt(void* page_table)
{
    
    asm volatile("lgdt %0" : "=r"(page_table));
    //return 10;
}