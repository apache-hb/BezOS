#include "stdlib.h"

void memset(u64* ptr, u64 len, u64 val)
{
    while(len)
        ptr[len--] = val;
}