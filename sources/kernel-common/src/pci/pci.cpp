#include "pci/pci.hpp"

#include "log.hpp"

#include <utility>

pci::Capability pci::ReadCapability(IConfigSpace *config, uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset) {
    uint32_t data = config->read32(bus, slot, function, offset);
    CapabilityId id = CapabilityId(data & 0xFF);
    uint8_t next = (data >> 8) & 0xFF;
    uint16_t control = (data >> 16) & 0xFFFF;

    return pci::Capability { id, next, control };
}

void pci::ReadCapabilityList(IConfigSpace *config, uint8_t bus, uint8_t slot, uint8_t function, ConfigHeader header) {
    uint8_t offset = config->read8(bus, slot, function, header.capabilityOffset()) & ~0b11;

    auto busId = km::format(km::Hex(bus).pad(2, '0', false));
    auto slotId = km::format(km::Hex(slot).pad(2, '0', false));
    auto functionId = km::format(km::Hex(function).pad(2, '0', false));

    int index = 0;

    while (true) {
        KmDebugMessage("| /SYS/PCI/", busId, ".", slotId, ".", functionId, "/C", index, " | Capability list      | ", km::Hex(offset).pad(2), "\n");

        pci::Capability capability = pci::ReadCapability(config, bus, slot, function, offset);

        KmDebugMessage("| /SYS/PCI/", busId, ".", slotId, ".", functionId, "/C", index, " | Capability ID        | ", capability.id, "\n");
        KmDebugMessage("| /SYS/PCI/", busId, ".", slotId, ".", functionId, "/C", index, " | Next offset          | ", capability.next, "\n");
        KmDebugMessage("| /SYS/PCI/", busId, ".", slotId, ".", functionId, "/C", index, " | Capability           | ", capability.control, "\n");

        if (capability.next == 0)
            break;

        offset = capability.next;
        index += 1;
    }
}

pci::ConfigHeader pci::QueryHeader(IConfigSpace *config, uint8_t bus, uint8_t slot, uint8_t function) {
    uint16_t vendor = config->read16(bus, slot, function, 0x0);
    if (vendor == 0xFFFF) {
        return pci::ConfigHeader { DeviceId::eInvalid, VendorId::eInvalid };
    }

    uint16_t device = config->read16(bus, slot, function, 0x2);
    uint16_t command = config->read16(bus, slot, function, 0x4);
    uint16_t status = config->read16(bus, slot, function, 0x6);

    uint8_t cls = config->read8(bus, slot, function, 0x8 + 3);
    uint8_t subclass = config->read8(bus, slot, function, 0x8 + 2);
    uint8_t programmable = config->read8(bus, slot, function, 0x8 + 1);
    uint8_t revision = config->read8(bus, slot, function, 0x8 + 0);

    uint8_t headerType = config->read8(bus, slot, function, 0xC + 2);

    return pci::ConfigHeader {
        .deviceId = DeviceId(device),
        .vendorId = VendorId(vendor),
        .command = command,
        .status = DeviceStatus(status),

        .cls = DeviceClass(DeviceClassCode(cls), subclass),
        .programmable = programmable,
        .revision = revision,
        .type = DeviceType(headerType),
    };
}

pci::BridgeConfig pci::QueryBridge(IConfigSpace *config, uint8_t bus, uint8_t slot, uint8_t function) {
    uint32_t bar0 = config->read16(bus, slot, function, 0x10);
    uint32_t bar1 = config->read16(bus, slot, function, 0x14);
    uint8_t subordinate = config->read8(bus, slot, function, 0x19);
    uint8_t secondary = config->read8(bus, slot, function, 0x1A);
    uint8_t primary = config->read8(bus, slot, function, 0x1B);

    return pci::BridgeConfig {
        .bar0 = bar0,
        .bar1 = bar1,
        .primaryBus = primary,
        .secondaryBus = secondary,
        .subordinateBus = subordinate,
    };
}

static void ProbePciBus(pci::IConfigSpace *config, uint8_t bus);
static void ProbePciDevice(pci::IConfigSpace *config, uint8_t bus, uint8_t slot);

static pci::ConfigHeader ProbePciFunction(pci::IConfigSpace *config, uint8_t bus, uint8_t slot, uint8_t function) {
    pci::ConfigHeader header = pci::QueryHeader(config, bus, slot, function);

    if (!header.isValid()) return header;

    auto busId = km::format(km::Hex(bus).pad(2, '0', false));
    auto slotId = km::format(km::Hex(slot).pad(2, '0', false));
    auto functionId = km::format(km::Hex(function).pad(2, '0', false));

    KmDebugMessage("| /SYS/PCI/", busId, ".", slotId, ".", functionId, "    | Vendor ID            | ", header.vendorId, "\n");
    KmDebugMessage("| /SYS/PCI/", busId, ".", slotId, ".", functionId, "    | Device ID            | ", header.deviceId, "\n");
    KmDebugMessage("| /SYS/PCI/", busId, ".", slotId, ".", functionId, "    | Command              | ", km::Hex(header.command).pad(4), "\n");
    KmDebugMessage("| /SYS/PCI/", busId, ".", slotId, ".", functionId, "    | Class                | ", header.cls, "\n");
    KmDebugMessage("| /SYS/PCI/", busId, ".", slotId, ".", functionId, "    | Programmable         | ", header.programmable, "\n");
    KmDebugMessage("| /SYS/PCI/", busId, ".", slotId, ".", functionId, "    | Revision             | ", header.revision, "\n");
    KmDebugMessage("| /SYS/PCI/", busId, ".", slotId, ".", functionId, "    | Type                 | ", header.type, "\n");
    KmDebugMessage("| /SYS/PCI/", busId, ".", slotId, ".", functionId, "    | Status               | ", km::Hex(std::to_underlying(header.status)).pad(4), "\n");

    if (header.hasCapabilityList()) {
        KmDebugMessage("| /SYS/PCI/", busId, ".", slotId, ".", functionId, "    | Capability list head | ", km::Hex(header.capabilityOffset()), "\n");
        ReadCapabilityList(config, bus, slot, function, header);
    }

    if (header.isPciBridge()) {
        pci::BridgeConfig bridge = pci::QueryBridge(config, bus, slot, function);
        ProbePciBus(config, bridge.secondaryBus);
    }

    return header;
}

static void ProbePciBus(pci::IConfigSpace *config, uint8_t bus) {
    for (uint8_t slot = 0; slot < 32; slot++) {
        ProbePciDevice(config, bus, slot);
    }
}

static void ProbePciDevice(pci::IConfigSpace *config, uint8_t bus, uint8_t slot) {
    pci::ConfigHeader header = ProbePciFunction(config, bus, slot, 0);
    if (!header.isValid()) return;

    if (header.isMultiFunction()) {
        for (uint8_t function = 1; function < 8; function++) {
            ProbePciFunction(config, bus, slot, function);
        }
    }
}

static void ProbeBusRange(pci::IConfigSpace *config, uint8_t first, uint8_t last) {
    for (uint8_t bus = first; bus < last; bus++) {
        ProbePciBus(config, bus);
    }
}

void pci::ProbeConfigSpace(IConfigSpace *config, const acpi::Mcfg *mcfg) {
    KmDebugMessage("| PCI                 | Property              | Value\n");
    KmDebugMessage("|---------------------+-----------------------+--------------------\n");

    if (mcfg == nullptr) {
        ProbeBusRange(config, 0, 255);
    } else {
        for (const acpi::McfgAllocation& allocation : mcfg->mcfgAllocations()) {
            ProbeBusRange(config, allocation.startBusNumber, allocation.endBusNumber);
        }
    }
}

using DeviceIdFormat = km::Format<pci::DeviceId>;
using VendorIdFormat = km::Format<pci::VendorId>;
using DeviceTypeFormat = km::Format<pci::DeviceType>;
using DeviceClassCodeFormat = km::Format<pci::DeviceClassCode>;
using DeviceClassFormat = km::Format<pci::DeviceClass>;
using CapabilityIdFormat = km::Format<pci::CapabilityId>;

using namespace stdx::literals;

DeviceIdFormat::String DeviceIdFormat::toString(pci::DeviceId id) {
    switch (id) {
    case pci::DeviceId::eInvalid:
        return "Invalid (0xFFFF)";
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
        return "Invalid (0xFFFF)";
    case eIntel:
        return "Intel (0x8086)";
    case eAMD:
        return "Advanced Micro Devices (0x1022)";
    case eATI:
        return "ATI Technologies (0x1002)";
    case eNvidia:
        return "NVidia Corporation (0x10DE)";
    case eMicron:
        return "Micron Technology (0x12B9)";
    case eRealtek:
        return "Realtek Semiconductor Corporation (0x10EC)";
    case eMediaTek:
        return "MediaTek Incorporation (0x14C3)";
    case eQemuVirtio:
        return "QEMU Virtio (0x1AF4)";
    case eQemu:
        return "QEMU (0x1B36)";
    case eQemuVga:
        return "QEMU stdvga (0x1234)";
    case eOracle:
        return "Oracle Corporation (0x108E)";
    case eBroadcom:
        return "Broadcom Limited (0x1166)";
    case eSandisk:
        return "Sandisk Corp (0x15B7)";
    default:
        return km::format(km::Hex(std::to_underlying(id)).pad(4, '0'));
    }
}

void DeviceTypeFormat::format(km::IOutStream& out, pci::DeviceType type) {
    switch (type & ~pci::DeviceType::eMultiFunction) {
    case pci::DeviceType::eGeneral:
        out.write("General");
        break;
    case pci::DeviceType::eCardBusBridge:
        out.write("CardBus Bridge");
        break;
    case pci::DeviceType::ePciBridge:
        out.write("PCI Bridge");
        break;
    default:
        out.write(km::format(km::Hex(std::to_underlying(type)).pad(2, '0')));
        break;
    }

    if (bool(type & pci::DeviceType::eMultiFunction)) {
        out.write(" (Multi-Function)");
    }
}

DeviceClassCodeFormat::String DeviceClassCodeFormat::toString(pci::DeviceClassCode cls) {
    switch (cls) {
    case pci::DeviceClassCode::eUnclassified:
        return "Unclassified";
    default:
        return km::format(km::Hex(std::to_underlying(cls)).pad(2, '0'));
    }
}

DeviceClassFormat::String DeviceClassFormat::toString(pci::DeviceClass cls) {
    switch (cls.cls) {
    case pci::DeviceClassCode::eUnclassified: {
        switch (cls.subclass) {
        case 0x00:
            return "Non-VGA-Compatible device";
        case 0x01:
            return "VGA-Compatible device";
        default:
            break;
        }

        DeviceClassFormat::String code;
        code.add("Unclassified (subclass: ");
        code.add(km::format(km::Hex(cls.subclass).pad(2, '0')));
        code.add(")");
        return code;
    }
    default: {
        DeviceClassFormat::String code;
        code.add("DeviceClass(class: ");
        code.add(DeviceClassCodeFormat::toString(cls.cls));
        code.add(", subclass: ");
        code.add(km::format(km::Hex(cls.subclass).pad(2, '0')));
        code.add(")");
        return code;
    }
    }
}

void CapabilityIdFormat::format(km::IOutStream& out, pci::CapabilityId value) {
    switch (value) {
    case pci::CapabilityId::eNull:
        out.write("Null (0x00)");
        break;
    case pci::CapabilityId::ePowerManagement:
        out.write("Power Management (0x01)");
        break;
    case pci::CapabilityId::eMsi:
        out.write("MSI (0x05)");
        break;
    case pci::CapabilityId::ePciExpress:
        out.write("PCIe (0x10)");
        break;
    case pci::CapabilityId::eMsiX:
        out.write("MSI-X (0x11)");
        break;
    case pci::CapabilityId::eSataConfig:
        out.write("SATA Configuration (0x12)");
        break;
    default:
        out.write(km::format(km::Hex(std::to_underlying(value)).pad(2, '0')));
        break;
    }
}
