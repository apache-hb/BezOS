typedef unsigned short uint16_t;

void kmain(void) {
    uint16_t* vga = (uint16_t*)0xB8000;
    vga[1] = 'a' | ( 1 | 0 << 4) << 8;
}