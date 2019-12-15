#include "vga.h"
#include "types.h"
 
using namespace bezos;

namespace besoz::vga
{

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_BUFFER 0xB80000

    enum class vga_colour : u16
    {
        black = 0,
        blue = 1,
        green = 2,
        cyan = 3,
        red = 4,
        magenta = 5,
        brown = 6,
        light_grey = 7,
        dark_grey = 8,
        light_blue = 9,
        light_green = 10,
        light_cyan = 11,
        light_red = 12,
        light_magenta = 13,
        light_brown = 14,
        white = 15,
    };

    u8 colour;
    u16* buffer;
    size row;
    size column;

    void init()
    {
        row = 0;
        column = 0;
        buffer = (u16*)VGA_BUFFER;
    }

    u8 make_colour(u16 front, u16 back)
    {
        return front | back << 4;
    }

    u16 vga_entry(u16 c, u16 colour)
    {
        return (u16) c | colour << 8;
    }

    void putchar(char c)
    {

    }

    void print(const char* str)
    {
        while(*str)
        {
            if(*str == '\n')
            {
                row++;
                column = 0;
                str++;
                continue;
            }

            putchar(*str);
            str++;
        }
    }
}