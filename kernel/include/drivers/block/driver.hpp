#pragma once

#include "pci/pci.hpp"

#include <new>
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

        uint64_t size() const {
            return blockSize * blockCount;
        }
    };

    enum class BlockDeviceStatus {
        eOk,
        eReadOnly,
        eInternalError,
        eOutOfRange,
    };

    class IBlockDriver {
        virtual BlockDeviceStatus readImpl(uint64_t block, void *buffer, size_t count) = 0;
        virtual BlockDeviceStatus writeImpl(uint64_t block, const void *buffer, size_t count) = 0;

    public:
        virtual ~IBlockDriver() = default;

        void operator delete(IBlockDriver*, std::destroying_delete_t) {
            std::unreachable();
        }

        virtual BlockDeviceCapability capability() const = 0;

        BlockDeviceStatus read(uint64_t block, void *buffer, size_t count);
        BlockDeviceStatus write(uint64_t block, const void *buffer, size_t count);
    };

    namespace detail {
        struct SectorRange {
            uint64_t firstSector;
            size_t firstSectorOffset;
            uint64_t lastSector;
            size_t lastSectorOffset;
        };

        SectorRange SectorRangeForSpan(size_t offset, size_t size, const BlockDeviceCapability &cap);
    }

    class BlockDevice {
        IBlockDriver *mDevice;
        std::unique_ptr<std::byte[]> mBuffer;

    public:
        BlockDevice(IBlockDriver *device [[gnu::nonnull]]);

        BlockDeviceCapability capability() const { return mDevice->capability(); }
        size_t size() const { return capability().size(); }
        uint64_t blockSize() const { return capability().blockSize; }

        size_t read(size_t offset, void *buffer, size_t size);
        size_t write(size_t offset, const void *buffer, size_t size);

        IBlockDriver *device() const { return mDevice; }

        void sync();
    };

    using DeviceFactory = IBlockDriver *(*)(void *);

    struct BlockDeviceDriver {
        pci::VendorId vendorId;
        pci::DeviceId deviceId;
        size_t size;
        size_t align;
        DeviceFactory factory;
    };

    class PartitionBlockDevice final : public IBlockDriver {
        IBlockDriver *mDevice;
        uint64_t mFront;
        uint64_t mBack;

        BlockDeviceStatus readImpl(uint64_t block, void *buffer, size_t count) override;
        BlockDeviceStatus writeImpl(uint64_t block, const void *buffer, size_t count) override;

    public:
        PartitionBlockDevice(IBlockDriver *device, uint64_t front, uint64_t back);

        BlockDeviceCapability capability() const override;
    };
}
