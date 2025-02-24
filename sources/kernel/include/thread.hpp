#pragma once

#include "arch/msr.hpp"
#include "memory.hpp"

#define CPU_LOCAL [[gnu::section(".cpudata")]]

namespace km {
    constexpr x64::ModelRegister<0xC0000100, x64::RegisterAccess::eReadWrite> kFsBase;
    constexpr x64::ModelRegister<0xC0000101, x64::RegisterAccess::eReadWrite> kGsBase;
    constexpr x64::ModelRegister<0xC0000102, x64::RegisterAccess::eReadWrite> kKernelGsBase;

    struct CpuLocalRegisters {
        uint64_t fsBase;
        uint64_t gsBase;
        uint64_t kernelGsBase;
    };

    CpuLocalRegisters LoadTlsRegisters();
    void StoreTlsRegisters(CpuLocalRegisters registers);

    void *GetCpuLocalData(void *object);
    bool IsCpuStorageSetup();
    uint64_t GetCpuStorageOffset(const void *object);

    template<typename T>
    class CpuLocal {
        alignas(T) std::byte mStorage[sizeof(T)]{};

    public:
        constexpr CpuLocal() = default;

        T& get() {
            return *reinterpret_cast<T*>(GetCpuLocalData(this));
        }

        T *operator&() {
            return &get();
        }

        T *operator->() {
            return &get();
        }

        void operator=(const T& value) {
            get() = value;
        }

        size_t tlsOffset() const {
            return GetCpuStorageOffset(this);
        }
    };

    /// @brief Get the size of a cpu tls data block.
    ///
    /// @return The size of a cpu tls data block.
    size_t CpuLocalDataSize();

    void *AllocateCpuLocalRegion();

    void InitCpuLocalRegion();
}
