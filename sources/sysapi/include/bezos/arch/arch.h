#pragma once

#if defined(__x86_64__)
#   include <bezos/arch/x86_64/context.h>
#else
#   error "Unsupported architecture"
#endif
