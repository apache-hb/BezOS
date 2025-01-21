#include "pci.hpp"
#include "delay.hpp"
#include "log.hpp"

#include <utility>

static constexpr uint8_t kOffsetMask = 0b1111'1100;

static constexpr uint32_t kEnableBit = (1 << 31);

static constexpr uint16_t kAddressPort = 0xCF8;
static constexpr uint16_t kDataPort = 0xCFC;

uint32_t pci::ReadConfigLong(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset) {
    uint32_t address
        = uint32_t(bus) << 16
        | uint32_t(slot) << 11
        | uint32_t(function) << 8
        | uint32_t(offset & kOffsetMask)
        | kEnableBit;

    KmWriteLong(kAddressPort, address);

    return KmReadLong(kDataPort);
}

uint16_t pci::ReadConfigWord(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset) {
    uint32_t data = pci::ReadConfigLong(bus, slot, function, offset);
    return (data >> ((offset & 2) * 8)) & 0xFFFF;
}

uint8_t pci::ReadConfigByte(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset) {
    uint32_t data = pci::ReadConfigLong(bus, slot, function, offset);
    uint32_t shift = (3 - (offset % 4)) * 8;
    uint8_t result = (data >> shift) & 0xFF;
    return result;
}

pci::ConfigHeader pci::QueryConfig(uint8_t bus, uint8_t slot, uint8_t function) {
    uint16_t vendor = ReadConfigWord(bus, slot, function, 0x0);
    if (vendor == 0xFFFF) {
        return pci::ConfigHeader { DeviceId::eInvalid, VendorId::eInvalid };
    }

    uint16_t device = ReadConfigWord(bus, slot, function, 0x2);
    uint16_t status = ReadConfigWord(bus, slot, function, 0x4);

    uint8_t cls = ReadConfigByte(bus, slot, function, 0x8);
    uint8_t subclass = ReadConfigByte(bus, slot, function, 0x9);
    uint8_t programmable = ReadConfigByte(bus, slot, function, 0xA);
    uint8_t revision = ReadConfigByte(bus, slot, function, 0xB);

    uint8_t headerType = ReadConfigByte(bus, slot, function, 0xD);

    return pci::ConfigHeader {
        .deviceId = DeviceId(device),
        .vendorId = VendorId(vendor),
        .status = DeviceStatus(status),

        .cls = DeviceClass(DeviceClassCode(cls), subclass),
        .programmable = programmable,
        .revision = revision,
        .type = DeviceType(headerType),
    };
}

pci::BridgeConfig pci::QueryBridge(uint8_t bus, uint8_t slot, uint8_t function) {
    uint32_t bar0 = ReadConfigWord(bus, slot, function, 0x10);
    uint32_t bar1 = ReadConfigWord(bus, slot, function, 0x14);
    uint8_t subordinate = ReadConfigByte(bus, slot, function, 0x19);
    uint8_t secondary = ReadConfigByte(bus, slot, function, 0x1A);
    uint8_t primary = ReadConfigByte(bus, slot, function, 0x1B);

    return pci::BridgeConfig {
        .bar0 = bar0,
        .bar1 = bar1,
        .primaryBus = primary,
        .secondaryBus = secondary,
        .subordinateBus = subordinate,
    };
}

static void ProbePciBus(uint8_t bus);
static void ProbePciDevice(uint8_t bus, uint8_t slot);

static pci::ConfigHeader ProbePciFunction(uint8_t bus, uint8_t slot, uint8_t function) {
    pci::ConfigHeader header = pci::QueryConfig(bus, slot, function);

    if (header.isValid()) {
        auto busId = km::format(km::Hex(bus).pad(2, '0', false));
        auto slotId = km::format(km::Hex(slot).pad(2, '0', false));
        auto functionId = km::format(km::Hex(function).pad(2, '0', false));

        KmDebugMessage("| /SYS/PCI/", busId, ".", slotId, ".", functionId, " | Vendor ID            | ", header.vendorId, "\n");
        KmDebugMessage("| /SYS/PCI/", busId, ".", slotId, ".", functionId, " | Device ID            | ", header.deviceId, "\n");
        KmDebugMessage("| /SYS/PCI/", busId, ".", slotId, ".", functionId, " | Class                | ", header.cls, "\n");
        KmDebugMessage("| /SYS/PCI/", busId, ".", slotId, ".", functionId, " | Programmable         | ", header.programmable, "\n");
        KmDebugMessage("| /SYS/PCI/", busId, ".", slotId, ".", functionId, " | Revision             | ", header.revision, "\n");
        KmDebugMessage("| /SYS/PCI/", busId, ".", slotId, ".", functionId, " | Type                 | ", header.type, "\n");
    }

    if (header.isPciBridge()) {
        pci::BridgeConfig bridge = pci::QueryBridge(bus, slot, function);
        ProbePciBus(bridge.secondaryBus);
    }

    return header;
}

static void ProbePciBus(uint8_t bus) {
    for (uint8_t slot = 0; slot < 32; slot++) {
        ProbePciDevice(bus, slot);
    }
}

static void ProbePciDevice(uint8_t bus, uint8_t slot) {
    pci::ConfigHeader header = ProbePciFunction(bus, slot, 0);
    if (!header.isValid()) return;

    if (header.isMultiFunction()) {
        for (uint8_t function = 1; function < 8; function++) {
            ProbePciFunction(bus, slot, function);
        }
    }
}

void pci::ProbeConfigSpace() {
    for (uint8_t bus = 0; bus < 255; bus++) {
        ProbePciBus(bus);
    }
}

using DeviceIdFormat = km::Format<pci::DeviceId>;
using VendorIdFormat = km::Format<pci::VendorId>;
using DeviceTypeFormat = km::Format<pci::DeviceType>;
using DeviceClassCodeFormat = km::Format<pci::DeviceClassCode>;
using DeviceClassFormat = km::Format<pci::DeviceClass>;

using namespace stdx::literals;

DeviceIdFormat::String DeviceIdFormat::toString(pci::DeviceId id) {
    switch (id) {
    case pci::DeviceId::eInvalid:
        return "Invalid (0xFFFF)"_sv;
    default:
        return km::format(km::Hex(std::to_underlying(id)).pad(4, '0'));
    }
}

VendorIdFormat::String VendorIdFormat::toString(pci::VendorId id) {
    // Names sourced from https://pcisig.com/membership/member-companies
    // and https://devicehunt.com/all-pci-vendors
    switch (id) {
    using enum pci::VendorId;
    case eInvalid:
        return "Invalid (0xFFFF)"_sv;
    case eIntel:
        return "Intel (0x8086)"_sv;
    case eAMD:
        return "Advanced Micro Devices (0x1022)"_sv;
    case eATI:
        return "ATI Technologies (0x1002)"_sv;
    case eNvidia:
        return "NVidia Corporation (0x10DE)"_sv;
    case eMicron:
        return "Micron Technology (0x12B9)"_sv;
    case eRealtek:
        return "Realtek Semiconductor Corporation (0x10EC)"_sv;
    case eMediaTek:
        return "MediaTek Incorporation (0x14C3)"_sv;
    case eQemuVirtio:
        return "QEMU Virtio (0x1AF4)"_sv;
    case eQemu:
        return "QEMU (0x1B36)"_sv;
    case eQemuVga:
        return "QEMU stdvga (0x1234)"_sv;
    case eOracle:
        return "Oracle Corporation (0x108E)"_sv;
    case eBroadcom:
        return "Broadcom Limited (0x1166)"_sv;
    case eSandisk:
        return "Sandisk Corp (0x15B7)"_sv;
    default:
        return km::format(km::Hex(std::to_underlying(id)).pad(4, '0'));
    }
}

DeviceTypeFormat::String DeviceTypeFormat::toString(pci::DeviceType type) {
    switch (type) {
    case pci::DeviceType::eGeneral:
        return "General"_sv;
    case pci::DeviceType::eCardBusBridge:
        return "CardBus Bridge"_sv;
    case pci::DeviceType::ePciBridge:
        return "PCI Bridge"_sv;
    case pci::DeviceType::eMultiFunction:
        return "Multi-Function"_sv;
    default:
        return km::format(km::Hex(std::to_underlying(type)).pad(2, '0'));
    }
}

DeviceClassCodeFormat::String DeviceClassCodeFormat::toString(pci::DeviceClassCode cls) {
    switch (cls) {
    case pci::DeviceClassCode::eUnclassified:
        return "Unclassified"_sv;
    default:
        return km::format(km::Hex(std::to_underlying(cls)).pad(2, '0'));
    }
}

DeviceClassFormat::String DeviceClassFormat::toString(pci::DeviceClass cls) {
    switch (cls.cls) {
    case pci::DeviceClassCode::eUnclassified: {
        switch (cls.subclass) {
        case 0x00:
            return "Non-VGA-Compatible device"_sv;
        case 0x01:
            return "VGA-Compatible device"_sv;
        default:
            break;
        }

        DeviceClassFormat::String code;
        code.add("Unclassified (subclass: "_sv);
        code.add(km::format(km::Hex(cls.subclass).pad(2, '0')));
        code.add(")"_sv);
        return code;
    }
    default: {
        DeviceClassFormat::String code;
        code.add("DeviceClass(class: "_sv);
        code.add(DeviceClassCodeFormat::toString(cls.cls));
        code.add(", subclass: "_sv);
        code.add(km::format(km::Hex(cls.subclass).pad(2, '0')));
        code.add(")"_sv);
        return code;
    }
    }
}
