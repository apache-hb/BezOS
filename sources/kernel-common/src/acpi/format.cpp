#include "acpi/acpi.hpp"

#include <stddef.h>

using namespace stdx::literals;

using IapcFormat = km::Format<acpi::IapcBootArch>;
using AddrSpaceFormat = km::Format<acpi::AddressSpaceId>;
using AccessFormat = km::Format<acpi::AccessSize>;
using AddressFormat = km::Format<acpi::GenericAddress>;

IapcFormat::String IapcFormat::toString(acpi::IapcBootArch arch) {
    IapcFormat::String result;
    bool first = true;

    if (bool(arch & acpi::IapcBootArch::eLegacyDevices)) {
        result.add("Legacy devices");
        first = false;
    }

    if (bool(arch & acpi::IapcBootArch::e8042Controller)) {
        if (!first) {
            result.add(", ");
        }

        result.add("8042 controller");
        first = false;
    }

    if (bool(arch & acpi::IapcBootArch::eVgaNotPresent)) {
        if (!first) {
            result.add(", ");
        }

        result.add("VGA not present");
        first = false;
    }

    if (bool(arch & acpi::IapcBootArch::eMsiNotPresent)) {
        if (!first) {
            result.add(", ");
        }

        result.add("MSI not supported");
        first = false;
    }

    if (first) {
        return km::format(km::Hex(std::to_underlying(arch)).pad(2, '0'));
    }

    return result;
}

AddrSpaceFormat::String AddrSpaceFormat::toString(acpi::AddressSpaceId space) {
    switch (space) {
    case acpi::AddressSpaceId::eSystemMemory:
        return "RAM";
    case acpi::AddressSpaceId::eSystemIo:
        return "IO";
    case acpi::AddressSpaceId::ePciConfig:
        return "PCI";
    case acpi::AddressSpaceId::eEmbeddedController:
        return "IC";
    case acpi::AddressSpaceId::eSmbus:
        return "SMB";
    case acpi::AddressSpaceId::eSystemCmos:
        return "CMOS";
    case acpi::AddressSpaceId::ePciBarTarget:
        return "BAR";
    case acpi::AddressSpaceId::eIpmi:
        return "IPMI";
    case acpi::AddressSpaceId::eGeneralPurposeIo:
        return "GPIO";
    case acpi::AddressSpaceId::eGenericSerialBus:
        return "UART";
    case acpi::AddressSpaceId::ePlatformCommChannel:
        return "PCC";
    case acpi::AddressSpaceId::eFunctionalFixedHardware:
        return "FFH";
    default:
        return km::format(km::Hex(std::to_underlying(space)).pad(2, '0'));
    }
}

AccessFormat::String AccessFormat::toString(acpi::AccessSize size) {
    switch (size) {
    case acpi::AccessSize::eByte:
        return "Byte";
    case acpi::AccessSize::eWord:
        return "Word";
    case acpi::AccessSize::eDword:
        return "Dword";
    case acpi::AccessSize::eQword:
        return "Qword";
    default:
        return km::format(km::Hex(std::to_underlying(size)).pad(2, '0'));
    }
}

void AddressFormat::format(km::IOutStream& out, acpi::GenericAddress value) {
    auto as = km::format(value.addressSpace);
    for (size_t i = 0; i < 4 - as.count(); i++) {
        out.write(" ");
    }
    out.format(as, ":", km::Hex(value.address).pad(16, '0'));
    out.format("+", km::Int(value.offset).pad(3, '0'));
    out.format("[", km::Int(value.width).pad(3, '0'), "] ", value.accessSize);
}
