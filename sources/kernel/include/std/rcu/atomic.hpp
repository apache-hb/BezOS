#pragma once

#include "std/detail/counted.hpp"

namespace sm {
    template<typename T>
    class AtomicSharedPtr {
        using Counted = sm::detail::CountedObject<T>;
        std::atomic<Counted*> mControl;

    public:
        constexpr AtomicSharedPtr() noexcept = default;

    };
}
