#pragma once

namespace vga
{
    void init();

    void putc(char c);
    void puts(const char* str);
    void puti(int val, int base);
}
