#ifndef MM_MEMORY_H
#define MM_MEMORY_H 1

#include "common/types.h"

// get all e820 data and stuff
void memory_init(void);

u64 memory_total(void);
u64 memory_total_usable(void);

#endif // MM_MEMORY_H
