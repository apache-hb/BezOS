#pragma once

#if defined(__x86_64__)
#   include "arch/x86_64/mmu.hpp"
#elif defined(__sparcv9__)
#   include "arch/sparcv9/mmu.hpp"
#else
#   error "Unsupported architecture"
#endif

namespace arch {

}
