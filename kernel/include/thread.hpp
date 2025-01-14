#pragma once

#include "arch/msr.hpp"
#include "memory.hpp"

namespace km {
    constexpr x64::ModelRegister<0xC0000100, x64::RegisterAccess::eReadWrite> kFsBase;
    constexpr x64::ModelRegister<0xC0000101, x64::RegisterAccess::eReadWrite> kGsBase;
    constexpr x64::ModelRegister<0xC0000102, x64::RegisterAccess::eReadWrite> kKernelGsBase;

    void *GetTlsData(void *object);

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
    };

    /// @brief Get the size of a cpu tls data block.
    ///
    /// @return The size of a cpu tls data block.
    size_t TlsDataSize();

    void *AllocateTlsRegion(SystemMemory& memory);

    void InitTlsRegion(SystemMemory& memory);
}

#define TLS(type) [[gnu::section(".tlsdata")]] constinit km::ThreadLocal<type>
