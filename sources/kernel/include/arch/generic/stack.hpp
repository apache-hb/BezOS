#pragma once

namespace arch {
    struct GenericStackWalk {
        template<typename F>
        [[gnu::error("Walk not implemented by platform")]]
        static void Walk(void **frame, F&& callback);
    };
}
