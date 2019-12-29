typedef unsigned char u8;
typedef unsigned short u16;

void kmain(void)
{
    volatile u16* vga = (u16*)0xB8000;
    for(int i = 0; i < 80 * 25; i++)
        vga[i] = 25;
    
    while(1);
}

void kpanic(void)
{
    volatile u16* vga = (u16*)0xB8000;
    for(int i = 0; i < 80 * 25; i++)
        vga[i] = 0xFFFF;
    
    while(1);
}