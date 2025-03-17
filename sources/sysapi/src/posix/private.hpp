#pragma once

#include <bezos/status.h>
#include <bezos/facility/debug.h>

#include <posix/errno.h>

namespace std {
    class source_location {
        struct __impl {
            const char *_M_file_name;
            const char *_M_function_name;
            unsigned _M_line;
            unsigned _M_column;
        };
        const __impl *impl = nullptr;

        using bsl = decltype(__builtin_source_location());
    public:
        static consteval source_location current(bsl loc = __builtin_source_location()) noexcept {
            source_location sl;
            sl.impl = static_cast<const __impl *>(loc);
            return sl;
        }

        constexpr source_location() noexcept = default;

        constexpr const char *file_name() const noexcept {
            return impl != nullptr ? impl->_M_file_name : "";
        }

        constexpr const char *function_name() const noexcept {
            return impl != nullptr ? impl->_M_function_name : "";
        }

        constexpr unsigned line() const noexcept {
            return impl != nullptr ? impl->_M_line : 0;
        }

        constexpr unsigned column() const noexcept {
            return impl != nullptr ? impl->_M_column : 0;
        }
    };
}

[[gnu::visibility("hidden")]]
void Unimplemented(int error = ENOSYS, std::source_location location = std::source_location::current());

[[gnu::visibility("hidden")]]
void Unimplemented(int error, const char *message, std::source_location location = std::source_location::current());

[[gnu::visibility("hidden")]]
void DebugLog(OsDebugMessageFlags Flags, const char *format, ...);

[[gnu::visibility("hidden")]]
void AssertOsSuccess(OsStatus status, const char *expr, std::source_location location = std::source_location::current());

#define ASSERT_OS_SUCCESS(expr) AssertOsSuccess(expr, #expr)
