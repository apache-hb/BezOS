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
        result.add("Legacy devices"_sv);
        first = false;
    }

    if (bool(arch & acpi::IapcBootArch::e8042Controller)) {
        if (!first) {
            result.add(", "_sv);
        }

        result.add("8042 controller"_sv);
        first = false;
    }

    if (bool(arch & acpi::IapcBootArch::eVgaNotPresent)) {
        if (!first) {
            result.add(", "_sv);
        }

        result.add("VGA not present"_sv);
        first = false;
    }

    if (bool(arch & acpi::IapcBootArch::eMsiNotPresent)) {
        if (!first) {
            result.add(", "_sv);
        }

        result.add("MSI not supported"_sv);
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
        return "RAM"_sv;
    case acpi::AddressSpaceId::eSystemIo:
        return "IO"_sv;
    case acpi::AddressSpaceId::ePciConfig:
        return "PCI"_sv;
    case acpi::AddressSpaceId::eEmbeddedController:
        return "IC"_sv;
    case acpi::AddressSpaceId::eSmbus:
        return "SMB"_sv;
    case acpi::AddressSpaceId::eSystemCmos:
        return "CMOS"_sv;
    case acpi::AddressSpaceId::ePciBarTarget:
        return "BAR"_sv;
    case acpi::AddressSpaceId::eIpmi:
        return "IPMI"_sv;
    case acpi::AddressSpaceId::eGeneralPurposeIo:
        return "GPIO"_sv;
    case acpi::AddressSpaceId::eGenericSerialBus:
        return "UART"_sv;
    case acpi::AddressSpaceId::ePlatformCommChannel:
        return "PCC"_sv;
    case acpi::AddressSpaceId::eFunctionalFixedHardware:
        return "FFH"_sv;
    default:
        return km::format(km::Hex(std::to_underlying(space)).pad(2, '0'));
    }
}

AccessFormat::String AccessFormat::toString(acpi::AccessSize size) {
    switch (size) {
    case acpi::AccessSize::eByte:
        return "Byte"_sv;
    case acpi::AccessSize::eWord:
        return "Word"_sv;
    case acpi::AccessSize::eDword:
        return "Dword"_sv;
    case acpi::AccessSize::eQword:
        return "Qword"_sv;
    default:
        return km::format(km::Hex(std::to_underlying(size)).pad(2, '0'));
    }
}

AddressFormat::String AddressFormat::toString(acpi::GenericAddress addr) {
    AddressFormat::String result;
    auto as = km::format(addr.addressSpace);
    for (size_t i = 0; i < 4 - as.count(); i++) {
        result.add(" "_sv);
    }
    result.add(as);
    result.add(":"_sv);
    result.add(km::format(km::PhysicalAddress(addr.address)));

    result.add("+"_sv);
    result.add(km::format(km::Int(addr.offset).pad(3, '0')));

    result.add("["_sv);
    result.add(km::format(km::Int(addr.width).pad(3, '0')));
    result.add("] "_sv);
    result.add(km::format(addr.accessSize));

    return result;
}
