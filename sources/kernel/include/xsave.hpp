#pragma once

#include <bezos/status.h>

#include "arch/xsave.hpp"

#include "hypervisor.hpp"

#include "util/format.hpp"

#include <stdint.h>

namespace km {
    enum class SaveMode {
        eNoSave,
        eFxSave,
        eXSave,
    };

    class IFpuSave {
    public:
        virtual ~IFpuSave() noexcept = default;

        virtual SaveMode mode() const noexcept = 0;
        virtual size_t size() const noexcept = 0;
        virtual void save(void *buffer) const noexcept = 0;
        virtual void restore(void *buffer) const noexcept = 0;
        virtual void init() const noexcept { }
    };

    struct XSaveConfig {
        SaveMode target;
        uint64_t features;
        const ProcessorInfo *cpuInfo;
    };

    IFpuSave *InitFpuSave(const XSaveConfig& config);

    void XSaveInitApCore();
    size_t XSaveSize();
    void XSaveStoreState(x64::XSave *area);
    void XSaveLoadState(x64::XSave *area);
}

template<>
struct km::Format<km::SaveMode> {
    static void format(km::IOutStream &out, km::SaveMode mode);
};
