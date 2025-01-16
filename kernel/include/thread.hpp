#pragma once

#include "arch/msr.hpp"
#include "memory.hpp"

namespace km {
    constexpr x64::ModelRegister<0xC0000100, x64::RegisterAccess::eReadWrite> kFsBase;
    constexpr x64::ModelRegister<0xC0000101, x64::RegisterAccess::eReadWrite> kGsBase;
    constexpr x64::ModelRegister<0xC0000102, x64::RegisterAccess::eReadWrite> kKernelGsBase;

    struct TlsRegisters {
        uint64_t fsBase;
        uint64_t gsBase;
        uint64_t kernelGsBase;
    };

    TlsRegisters LoadTlsRegisters();
    void StoreTlsRegisters(TlsRegisters registers);

    void *GetTlsData(void *object);
    uint64_t GetTlsOffset(const void *object);

    template<typename T>
    class ThreadLocal {
        alignas(T) std::byte mStorage[sizeof(T)]{};

    public:
        constexpr ThreadLocal() = default;

        T& get() {
            return *reinterpret_cast<T*>(GetTlsData(this));
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
            return GetTlsOffset(this);
        }
    };

    /// @brief Get the size of a cpu tls data block.
    ///
    /// @return The size of a cpu tls data block.
    size_t TlsDataSize();

    void *AllocateTlsRegion(SystemMemory& memory);

    void InitTlsRegion(SystemMemory& memory);
}

#define TLS(type) [[gnu::section(".tlsdata")]] constinit km::ThreadLocal<type>
