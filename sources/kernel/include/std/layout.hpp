#pragma once

#include <stddef.h>

namespace sm {
    struct Layout {
        size_t size;
        size_t align;

        template<typename T>
        static consteval Layout of() noexcept {
            return Layout{sizeof(T), alignof(T)};
        }
    };
}
