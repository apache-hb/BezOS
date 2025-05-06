#pragma once

#if defined(__x86_64__)
#   include "arch/x86_64/machine.hpp"
#elif defined(__sparcv9__)
#   include "arch/sparcv9/machine.hpp"
#else
#   error "Unsupported architecture"
#endif
