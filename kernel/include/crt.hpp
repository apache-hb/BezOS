#pragma once

#include <stddef.h>
#include <stdint.h>

void KmMemoryCopy(void *dst, const void *src, size_t size);

void KmMemorySet(void *dst, uint8_t value, size_t size);
