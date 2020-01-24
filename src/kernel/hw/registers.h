#ifndef HW_REGISTERS_H
#define HW_REGISTERS_H 1

#include "common/types.h"

extern u64 cr0_get();
extern u64 cr2_get();
extern u64 cr3_get();
extern u64 cr4_get();

extern void cr0_set(u64 val);
extern void cr2_set(u64 val);
extern void cr3_set(u64 val);
extern void cr4_set(u64 val);

#endif // HW_REGISTERS_H
