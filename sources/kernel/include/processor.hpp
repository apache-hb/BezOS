#pragma once

#include "apic.hpp"
#include "util/format.hpp"
#include <utility>

namespace km {
    using CpuCoreCount = uint32_t;

    enum class CpuCoreId : CpuCoreCount {
        eInvalid = 0xFFFF'FFFF
    };

    void InitKernelThread(Apic pic);

    CpuCoreId GetCurrentCoreId() noexcept [[clang::reentrant]];
    IApic *GetCpuLocalApic() noexcept [[clang::reentrant]];
}

template<>
struct km::Format<km::CpuCoreId> {
    static void format(km::IOutStream& out, km::CpuCoreId id) {
        using namespace stdx::literals;
        out.format("CPU", km::Int(std::to_underlying(id)).pad(3));
    }
};
