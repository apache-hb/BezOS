#pragma once

#include <stddef.h>

using ssize_t = ptrdiff_t;

#if __STDC_HOSTED__
#   include <assert.h>
#   define TEST_ASSERT(x) assert(x)
#else
#   define TEST_ASSERT(x) ((void)0)
#endif
