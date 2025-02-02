#pragma once

#include "pci/pci.hpp"

#include <new>
#include <span>
#include <utility>

namespace km {
    enum class Protection {
        eNone = 0,

        eRead = (1 << 0),
        eWrite = (1 << 1),

        eReadWrite = eRead | eWrite,
    };

    UTIL_BITFLAGS(Protection);

    struct BlockDeviceCapability {
        Protection protection;
        uint32_t blockSize;
        uint32_t blockCount;
    };

    enum class BlockDeviceStatus {
        eOk,
        eReadOnly,
        eInternalError,
    };

    class IBlockDevice {
    public:
        virtual ~IBlockDevice() = default;

        void operator delete(IBlockDevice*, std::destroying_delete_t) {
            std::unreachable();
        }

        virtual BlockDeviceCapability capability() const = 0;

        virtual BlockDeviceStatus read(uint64_t block, void *buffer, size_t count) = 0;
        virtual BlockDeviceStatus write(uint64_t block, const void *buffer, size_t count) = 0;
    };

    using DeviceFactory = IBlockDevice *(*)(void *);

    struct BlockDeviceDriver {
        pci::VendorId vendorId;
        pci::DeviceId deviceId;
        size_t size;
        size_t align;
        DeviceFactory factory;
    };
}

#define DEVICE_DRIVER(name) \
    extern "C" [[gnu::used, gnu::section(".kernel.drivers")]] \
    constinit const km::BlockDeviceDriver name

std::span<const km::BlockDeviceDriver> GetDeviceDrivers();
km::IBlockDevice *VirtIoBlockDevice();
km::IBlockDevice *MemoryBlockDevice();
