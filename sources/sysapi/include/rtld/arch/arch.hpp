#pragma once

#ifdef __x86_64__
#   include "rtld/arch/x86_64/relocation.hpp" // IWYU pragma: export
#else
#   error "Unsupported architecture"
#endif
