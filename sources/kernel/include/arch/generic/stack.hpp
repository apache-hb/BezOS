#pragma once

namespace arch {
    struct GenericStackWalk {
        template<typename F>
        [[gnu::error("GenericStackWalk::Walk not implemented by platform")]]
        static void Walk(void **frame, F&& callback);
    };
}
