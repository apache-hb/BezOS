#pragma once

#include "types.h"

#ifndef TARGET_X86
#   define TARGET_X86 0
#endif

#ifndef TARGET_ARM
#   define TARGET_ARM 0
#endif

extern "C" bezos::u64 KERNEL_END;