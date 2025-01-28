#pragma once

#include "pci.hpp"

#include <new>
#include <span>
#include <utility>

namespace km {
    struct BlockDeviceStatus {
        bool readSupport;
        bool writeSupport;
    };

    class IBlockDevice {
    public:
        virtual ~IBlockDevice() = default;

        void operator delete(IBlockDevice*, std::destroying_delete_t) {
            std::unreachable();
        }

        virtual BlockDeviceStatus getStatus() = 0;
    };

    struct DeviceDriver {
        pci::VendorId vendorId;
        pci::DeviceId deviceId;
    };
}

#define DEVICE_DRIVER(name) \
    extern "C" [[gnu::used, gnu::section(".kernel.drivers")]] \
    const km::DeviceDriver name

std::span<const km::DeviceDriver> GetDeviceDrivers();
