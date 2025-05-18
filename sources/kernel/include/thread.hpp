#pragma once

#include "arch/msr.hpp"
#include <cstddef>

#define CPU_LOCAL [[gnu::section(".cpudata")]]

constexpr x64::RwModelRegister<0xC0000100> IA32_FS_BASE;
constexpr x64::RwModelRegister<0xC0000101> IA32_GS_BASE;
constexpr x64::RwModelRegister<0xC0000102> IA32_KERNEL_GS_BASE;

namespace km {
    struct CpuLocalRegisters {
        uint64_t fsBase;
        uint64_t gsBase;
        uint64_t kernelGsBase;
    };

    CpuLocalRegisters LoadTlsRegisters();
    void StoreTlsRegisters(CpuLocalRegisters registers);

    void *GetCpuLocalData(void *object) noexcept [[clang::nonblocking, clang::reentrant]];
    bool IsCpuStorageSetup() noexcept [[clang::nonblocking, clang::reentrant]];
    uint64_t GetCpuStorageOffset(const void *object) noexcept [[clang::nonblocking, clang::reentrant]];

    template<typename T>
    class CpuLocal {
        alignas(T) std::byte mStorage[sizeof(T)]{};

    public:
        constexpr CpuLocal() = default;

        T& get() noexcept [[clang::nonblocking, clang::reentrant]] {
            return *reinterpret_cast<T*>(GetCpuLocalData(this));
        }

        T *operator&() noexcept [[clang::nonblocking, clang::reentrant]] {
            return &get();
        }

        T *operator->() noexcept [[clang::nonblocking, clang::reentrant]] {
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
