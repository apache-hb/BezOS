#ifndef MM_PAGING_H
#define MM_PAGING_H 1

#include "common/types.h"

typedef enum {
    page_size_4kb = 1,
    page_size_2mb = 2,
    page_size_1gb = 3,
} page_size_t;

typedef enum {
    page_present = (1 << 0),
    page_write = (1 << 1),
    page_user = (1 << 2),
    page_write_cache = (1 << 3),
    page_cache_disable = (1 << 4),
    page_accessed = (1 << 5),
    page_4mb = (1 << 7),
} page_modifier_t;

// initialize paging
// @top_page the initial pml4, used for setup
void paging_init();

// allocate n pages from memory
// @num number of pages to allocate
// @size size of each page allocated
void* page_alloc(u64 num, page_size_t size);

// map a physical memory address to a virtual memory address
// physical_addr has to be page aligned
void* map_physical_address(void* physical_addr, page_modifier_t mods);
#endif // MM_PAGING_H
