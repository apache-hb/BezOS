#pragma once

#include "apic.hpp"
#include "util/format.hpp"
#include <utility>

namespace km {
    enum class CpuCoreId : uint32_t {
        eInvalid = 0xFFFF'FFFF
    };

    void InitKernelThread(Apic pic);

    CpuCoreId GetCurrentCoreId();
    IApic *GetCurrentCoreApic();
}

template<>
struct km::Format<km::CpuCoreId> {
    static void format(km::IOutStream& out, km::CpuCoreId id) {
        using namespace stdx::literals;
        out.format("CPU"_sv, km::Int(std::to_underlying(id)).pad(3));
    }
};
