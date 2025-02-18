#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

void KmMemoryCopy(void *dst, const void *src, size_t size);

void KmMemorySet(void *dst, uint8_t value, size_t size);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

// libc headers define these prototypes only in C code
// In C++ I need to define them myself
extern "C" void *malloc(size_t size);
extern "C" void *realloc(void *old, size_t size);
extern "C" void free(void *ptr);

// aligned allocation
extern "C" void *aligned_alloc(size_t alignment, size_t size);

#endif
