#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void debug_prints(const char *message);
void debug_printull(uint64_t value);

#ifdef __cplusplus
}
#endif
