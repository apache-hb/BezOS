#include "log.h"

#define VGA_BUFFER ((u16*)0xB8000)

namespace log {
    u32 put(char c, u32 idx) {
        if (c == '\n')
            idx = ((idx + 80 - 1) / 80) * 80;

        if (idx > 80 * 25)
            idx = 0;

        VGA_BUFFER[idx] = c | 7 << 8;

        return idx++;
    }

    void vga::info(const char *str) {
        ((u16*)0xB8000)[0] = 'z' | 7 << 8;
        for (;;) { }
        while (*str)
            idx = put(*str++, idx);
    }

    void vga::fatal(const char *str) {
        while (*str)
            idx = put(*str++, idx);
    }
}
