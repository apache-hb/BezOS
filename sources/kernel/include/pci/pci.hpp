#pragma once

#include "pci/config.hpp"

#include "util/format.hpp"
#include "common/util/util.hpp"

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

        eAMD = 0x1022,
        eATI = 0x1002,

        eMicron = 0x1344,

        eNvidia = 0x10DE,

        eRealtek = 0x10EC,
        eMediaTek = 0x14C3,

        eQemuVirtio = 0x1af4,
        eQemu = 0x1b36,
        eQemuVga = 0x1234,

        eSandisk = 0x15B7,

        eOracle = 0x108e,
        eBroadcom = 0x1166,
    };

    enum class DeviceClassCode : uint8_t {
        eUnclassified = 0x0,

        eBridge = 0x6,
    };

    enum class DeviceStatus : uint16_t {
        eCapsList = (1 << 4),
    };

    UTIL_BITFLAGS(DeviceStatus);

    struct DeviceClass {
        DeviceClassCode cls;
        uint8_t subclass;

        constexpr bool operator==(const DeviceClass& other) const = default;
    };

    enum class CapabilityId : uint8_t {
        eNull = 0x00,
        ePowerManagement = 0x01,
        eMsi = 0x05,
        ePciExpress = 0x10,
        eMsiX = 0x11,
        eSataConfig = 0x12,
    };

    struct Capability {
        CapabilityId id;
        uint8_t next;
        uint16_t control;
    };

    struct ConfigHeader {
        DeviceId deviceId;
        VendorId vendorId;
        uint16_t command;
        DeviceStatus status;
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

        bool hasCapabilityList() const {
            return bool(status & DeviceStatus::eCapsList);
        }

        bool isPciBridge() const {
            return cls == DeviceClass { DeviceClassCode::eBridge, 0x4 };
        }

        uint32_t capabilityOffset() const {
            if ((type & ~DeviceType::eMultiFunction) == DeviceType::eCardBusBridge) {
                return 0x14;
            }

            return 0x34;
        }
    };

    struct BridgeConfig {
        uint32_t bar0;
        uint32_t bar1;

        uint8_t primaryBus;
        uint8_t secondaryBus;
        uint8_t subordinateBus;
    };

    Capability ReadCapability(IConfigSpace *config, uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset);
    void ReadCapabilityList(IConfigSpace *config, uint8_t bus, uint8_t slot, uint8_t function, ConfigHeader header);
    ConfigHeader QueryHeader(IConfigSpace *config, uint8_t bus, uint8_t slot, uint8_t function);
    BridgeConfig QueryBridge(IConfigSpace *config, uint8_t bus, uint8_t slot, uint8_t function);

    void ProbeConfigSpace(IConfigSpace *config, const acpi::Mcfg *mcfg);
}

template<>
struct km::Format<pci::DeviceId> {
    using String = stdx::StaticString<64>;
    static String toString(pci::DeviceId id);
};

template<>
struct km::Format<pci::VendorId> {
    using String = stdx::StaticString<64>;
    static String toString(pci::VendorId id);
};

template<>
struct km::Format<pci::DeviceType> {
    static void format(km::IOutStream& out, pci::DeviceType type);
};

template<>
struct km::Format<pci::DeviceClassCode> {
    using String = stdx::StaticString<64>;
    static String toString(pci::DeviceClassCode cls);
};

template<>
struct km::Format<pci::DeviceClass> {
    using String = stdx::StaticString<128>;
    static String toString(pci::DeviceClass cls);
};

template<>
struct km::Format<pci::CapabilityId> {
    static void format(km::IOutStream& out, pci::CapabilityId value);
};
