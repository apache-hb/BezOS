#pragma once

#ifdef __clang__
#   include "common/compiler/clang/compiler.hpp" // IWYU pragma: export
#else
#   error "Unsupported compiler"
#endif

#include "common/compiler/generic/define.hpp" // IWYU pragma: export
