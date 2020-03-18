#ifndef MM_H
#define MM_H

#include "common/types.h"

typedef void* mm_page_table_t;

void mm_init(void);

void* mm_alloc_page(mm_page_table_t table, uint32 pages);

#endif
