#pragma once

#include "rtld/arch/arch.hpp"

namespace os {
    class SparcV9Relocator : public IRelocator {
    public:
        size_t dynSize() const override;
        size_t relaSize() const override;
        size_t relSize() const override;
        OsStatus apply(void *data, size_t count) override;
    };
}
