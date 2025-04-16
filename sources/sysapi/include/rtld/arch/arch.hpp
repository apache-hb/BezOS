#pragma once

#include <bezos/status.h>

#include <stddef.h>

namespace os {
    class IRelocator {
    public:
        virtual ~IRelocator() = default;

        virtual size_t dynSize() const = 0;
        virtual size_t relaSize() const = 0;
        virtual size_t relSize() const = 0;
        virtual OsStatus apply(void *data, size_t count) = 0;
    };
}
