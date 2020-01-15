#pragma once

namespace bezos::mm
{
    void init();

    void* physical(void* addr);

    // map a physical address to a page
    void* map(void* phys);
}