#pragma once

#include "std/string_view.hpp"

#include <new>
#include <utility>

namespace km {
    class ILogTarget {
    public:
        virtual ~ILogTarget() = default;

        void operator delete(ILogTarget*, std::destroying_delete_t) noexcept {
            std::unreachable();
        }

        virtual void write(stdx::StringView message) noexcept = 0;
    };
}
