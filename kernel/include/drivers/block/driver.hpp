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
        uint64_t blockCount;
    };

    enum class BlockDeviceStatus {
        eOk,
        eReadOnly,
        eInternalError,
        eOutOfRange,
    };

    class IBlockDevice {
        virtual BlockDeviceStatus readImpl(uint64_t block, void *buffer, size_t count) = 0;
        virtual BlockDeviceStatus writeImpl(uint64_t block, const void *buffer, size_t count) = 0;

    public:
        virtual ~IBlockDevice() = default;

        void operator delete(IBlockDevice*, std::destroying_delete_t) {
            std::unreachable();
        }

        virtual BlockDeviceCapability capability() const = 0;

        BlockDeviceStatus read(uint64_t block, void *buffer, size_t count);
        BlockDeviceStatus write(uint64_t block, const void *buffer, size_t count);
    };

    class DriveMedia {
        IBlockDevice *mDevice;
        std::unique_ptr<std::byte[]> mBuffer;

    public:
        DriveMedia(IBlockDevice *device [[gnu::nonnull]]);

        size_t size() const;

        size_t read(size_t offset, void *buffer, size_t size);
        size_t write(size_t offset, const void *buffer, size_t size);

        void sync();
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
