#ifndef MM_PAGING_H
#define MM_PAGING_H 1

#include "common/types.h"

typedef enum {
    // read only memory
    // a page can always be read from
    read = 0,
    
    // writeable memory, implies read as well
    // bit 1 is the write enable bit
    write = (1 << 1),

    // executable memory
    // if NX (bit 63) is 1 then the page cannot be executed
    exec = (1 << 63)
} paging_access;

typedef u64 page_access_t;

#endif // MM_PAGING_H
