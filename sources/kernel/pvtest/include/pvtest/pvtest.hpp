#pragma once

#include <errno.h>
#include <error.h>

namespace pv {
    enum BbType {
        kBreakExec = 0x0,
        kBreakWrite = 0x1,
        kBreakData = 0x3,
    };

    enum BpSize {
        kBreakByte = 0x0,
        kBreakWord = 0x1,
        kBreakDWord = 0x3,
        kBreakQWord = 0x2,
    };

    [[gnu::no_instrument_function]]
    bool AddHwBreak(void *address, int bpnumber, void (*callback)(int)) noexcept;

    [[gnu::no_instrument_function]]
    bool RemoveHwBreak(int bpnumber) noexcept;

    [[noreturn, gnu::no_instrument_function]]
    void ExitFork(int status) noexcept;
}

#define PV_POSIX_ASSERT(x) do { \
    if (!(x)) { \
        error_at_line(EXIT_FAILURE, errno, __FILE__, __LINE__, "%s", #x); \
        std::unreachable(); \
    } \
} while (0)

#define PV_POSIX_CHECK(x) do { \
    if (int err = (x); err < 0) { \
        error_at_line(EXIT_FAILURE, errno, __FILE__, __LINE__, "%s", #x); \
        std::unreachable(); \
    } \
} while (0)
