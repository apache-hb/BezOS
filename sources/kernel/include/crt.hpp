#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((__nothrow__, __nonblocking__, __nonnull__))
void KmMemoryCopy(void *dst, const void *src, size_t size);

__attribute__((__nothrow__, __nonblocking__, __nonnull__))
void KmMemorySet(void *dst, uint8_t value, size_t size);

#ifdef __cplusplus
}
#endif
