#pragma once

#include "pci/pci.hpp"

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

        virtual BlockDeviceStatus getStatus() const = 0;
    };

    using DeviceFactory = IBlockDevice *(*)(void *);

    struct DeviceDriver {
        pci::VendorId vendorId;
        pci::DeviceId deviceId;
        size_t size;
        size_t align;
        DeviceFactory factory;
    };
}

#define DEVICE_DRIVER(name) \
    extern "C" [[gnu::used, gnu::section(".kernel.drivers")]] \
    constinit const km::DeviceDriver name

std::span<const km::DeviceDriver> GetDeviceDrivers();
