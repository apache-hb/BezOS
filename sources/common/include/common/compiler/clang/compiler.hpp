#pragma once

#include "common/compiler/generic/compiler.hpp"

namespace sm {
    struct ClangCompiler : GenericCompiler {
        static constexpr std::string_view GetName() noexcept {
            return __clang_version__;
        }

        static constexpr Version GetVersion() noexcept {
            return Version(__clang_major__, __clang_minor__, __clang_patchlevel__);
        }
    };

    using Compiler = ClangCompiler;
}

#define CLANG_PRAGMA(x) _Pragma(#x)

#define CLANG_DIAGNOSTIC_PUSH() \
    _Pragma("clang diagnostic push")

#define CLANG_DIAGNOSTIC_POP() \
    _Pragma("clang diagnostic pop")

#define CLANG_DIAGNOSTIC_IGNORE(name) \
    CLANG_PRAGMA(clang diagnostic ignored name)

#define DIAGNOSTIC_PUSH() CLANG_DIAGNOSTIC_PUSH()
#define DIAGNOSTIC_POP() CLANG_DIAGNOSTIC_POP()
#define DIAGNOSTIC_IGNORE(name) CLANG_DIAGNOSTIC_IGNORE(name)
