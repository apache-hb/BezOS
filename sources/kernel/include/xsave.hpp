#pragma once

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

    namespace detail {
        x64::XSave *EmptyXSave();
        void SetupXSave(SaveMode mode, uint64_t features);
    }

    class IFpuSave {
    public:
        virtual ~IFpuSave() noexcept = default;

        virtual SaveMode mode() const noexcept [[clang::nonblocking, clang::reentrant]] = 0;
        virtual size_t size() const noexcept [[clang::nonblocking, clang::reentrant]] = 0;
        virtual void save(void *buffer) const noexcept [[clang::nonblocking, clang::reentrant]] = 0;
        virtual void restore(void *buffer) const noexcept [[clang::nonblocking, clang::reentrant]] = 0;
        virtual void init() const noexcept { }
    };

    struct XSaveConfig {
        SaveMode target;
        uint64_t features;
        const ProcessorInfo *cpuInfo;
    };

    IFpuSave *initFpuSave(const XSaveConfig& config);

    void XSaveInitApCore();
    size_t XSaveSize() noexcept [[clang::nonblocking, clang::reentrant]];
    void XSaveStoreState(x64::XSave *area) noexcept [[clang::reentrant]];
    void XSaveLoadState(x64::XSave *area) noexcept [[clang::reentrant]];
    x64::XSave *CreateXSave();
    void DestroyXSave(x64::XSave *area);
}

template<>
struct km::Format<km::SaveMode> {
    static void format(km::IOutStream &out, km::SaveMode mode);
};
