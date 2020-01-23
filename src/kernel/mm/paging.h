#ifndef MM_PAGING_H
#define MM_PAGING_H 1

#include "common/types.h"

typedef enum {
    page_size_4kb = 1,
    page_size_2mb = 2,
    // this might be pointless to implement
    page_size_4mb = 3,
    page_size_1gb = 4,
    // this might not be supported yet
    page_size_512gb = 5,
} page_size_t;

// initialize paging
// @top_page the initial pml4, used for setup
void paging_init(void* top_page);

// allocate n pages from memory
// @num number of pages to allocate
// @size size of each page allocated
void* page_alloc(u64 num, page_size_t size);
#endif // MM_PAGING_H
