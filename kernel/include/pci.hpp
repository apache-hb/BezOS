#pragma once

#include "util/format.hpp"
#include "util/util.hpp"

#include <stdint.h>

namespace pci {
    enum class DeviceType : uint8_t {
        eGeneral = 0x0,
        ePciBridge = 0x1,
        eCardBusBridge = 0x2,

        eMultiFunction = (1 << 7),
    };

    UTIL_BITFLAGS(DeviceType);

    enum class DeviceId : uint16_t {
        eInvalid = 0xFFFF,
    };

    enum class VendorId : uint16_t {
        eInvalid = 0xFFFF,

        eIntel = 0x8086,

        eQemuVirtio = 0x1af4,
        eQemu = 0x1b36,

        eOracle = 0x108e,
        eBroadcom = 0x1166,
    };

    enum class DeviceClassCode : uint8_t {
        eUnclassified = 0x0,

        eBridge = 0x6,
    };

    struct DeviceClass {
        DeviceClassCode cls;
        uint8_t subclass;

        constexpr bool operator==(const DeviceClass& other) const = default;
    };

    struct ConfigHeader {
        DeviceId deviceId;
        VendorId vendorId;
        DeviceClass cls;
        uint8_t programmable;
        uint8_t revision;
        DeviceType type;

        bool isValid() const {
            return deviceId != DeviceId::eInvalid;
        }

        bool isMultiFunction() const {
            return bool(type & DeviceType::eMultiFunction);
        }

        bool isPciBridge() const {
            return cls == DeviceClass { DeviceClassCode::eBridge, 0x4 };
        }
    };

    struct BridgeConfig {
        uint32_t bar0;
        uint32_t bar1;

        uint8_t primaryBus;
        uint8_t secondaryBus;
        uint8_t subordinateBus;
    };

    struct Device {

    };

    uint32_t ReadConfigLong(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset);
    uint16_t ReadConfigWord(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset);
    uint8_t ReadConfigByte(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset);

    ConfigHeader QueryConfig(uint8_t bus, uint8_t slot, uint8_t function);
    BridgeConfig QueryBridge(uint8_t bus, uint8_t slot, uint8_t function);

    void ProbeConfigSpace();
}

template<>
struct km::StaticFormat<pci::DeviceId> {
    using String = stdx::StaticString<64>;
    static String toString(pci::DeviceId id);
};

template<>
struct km::StaticFormat<pci::VendorId> {
    using String = stdx::StaticString<64>;
    static String toString(pci::VendorId id);
};

template<>
struct km::StaticFormat<pci::DeviceType> {
    using String = stdx::StaticString<64>;
    static String toString(pci::DeviceType type);
};

template<>
struct km::StaticFormat<pci::DeviceClassCode> {
    using String = stdx::StaticString<64>;
    static String toString(pci::DeviceClassCode cls);
};

template<>
struct km::StaticFormat<pci::DeviceClass> {
    using String = stdx::StaticString<128>;
    static String toString(pci::DeviceClass cls);
};
