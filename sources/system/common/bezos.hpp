#pragma once

#include <bezos/facility/debug.h>

#include <format>

namespace os {
    template<typename... A>
    inline OsStatus debug_message(OsDebugMessageFlags flags, const std::format_string<A...>& fmt, A&&... args) {
        auto text = std::vformat(fmt.get(), std::make_format_args(args...));
        OsDebugMessageInfo message_info {
            .Front = text.data(),
            .Back = text.data() + text.size(),
            .Info = flags,
        };
        return OsDebugMessage(message_info);
    }
}
