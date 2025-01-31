#include "pci/config.hpp"

#include "delay.hpp"
#include "log.hpp"

static constexpr uint8_t kOffsetMask = 0b1111'1100;

static constexpr uint32_t kEnableBit = (1 << 31);

static constexpr uint16_t kAddressPort = 0xCF8;
static constexpr uint16_t kDataPort = 0xCFC;

static constexpr size_t kEcamSize = (256 * 32 * 8) * 0x1000;

// generic PCI configuration space read

uint16_t pci::IConfigSpace::read16(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset) {
    uint32_t data = read32(bus, slot, function, offset);
    return (data >> ((offset & 2) * 8)) & 0xFFFF;
}

uint8_t pci::IConfigSpace::read8(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset) {
    uint32_t data = read32(bus, slot, function, offset);
    uint32_t shift = (3 - (offset % 4)) * 8;
    uint8_t result = (data >> shift) & 0xFF;
    return result;
}

// port-based PCI configuration space read

uint32_t pci::PortConfigSpace::read32(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset) {
    uint32_t address
        = uint32_t(bus) << 16
        | uint32_t(slot) << 11
        | uint32_t(function) << 8
        | uint32_t(offset & kOffsetMask)
        | kEnableBit;

    KmWriteLong(kAddressPort, address);

    return KmReadLong(kDataPort);
}

// MMIO-based PCI configuration space read

pci::McfgConfigSpace::McfgConfigSpace(const acpi::Mcfg *mcfg, km::SystemMemory& memory)
    : mMcfg(mcfg)
    , mRegions(new EcamRegion[mMcfg->allocationCount()])
{
    for (size_t i = 0; i < mMcfg->allocationCount(); i++) {
        const acpi::McfgAllocation& allocation = mMcfg->allocations[i];

        EcamRegion region = {
            .first = allocation.startBusNumber,
            .last = allocation.endBusNumber,
            .base = memory.map(allocation.address, allocation.address + kEcamSize, km::PageFlags::eData, km::MemoryType::eWriteBack)
        };

        mRegions[i] = region;

        KmDebugMessage("[PCI] ECAM region ", i, ": ", (const void*)region.base, " (", (const void*)((char*)region.base + kEcamSize), ")", "\n");
    }
}

uint32_t pci::McfgConfigSpace::read32(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset) {
    uint32_t address = (((bus * 256) + (slot * 8) + function) * 0x1000) + offset;

    for (size_t i = 0; i < mMcfg->allocationCount(); i++) {
        EcamRegion region = mRegions[i];
        if (region.contains(bus)) {
            return *(uint32_t*)((uint8_t*)region.base + address);
        }
    }

    KmDebugMessage("[PCI] ECAM region not found for bus ", bus, ", slot ", slot, ", function ", function, "\n");
    return UINT32_MAX;
}

pci::IConfigSpace *pci::InitConfigSpace(const acpi::Mcfg *mcfg, km::SystemMemory& memory) {
    if (mcfg == nullptr) {
        KmDebugMessage("[PCI] No MCFG table.\n");
        return new PortConfigSpace{};
    }

    if (mcfg->allocationCount() == 0) {
        KmDebugMessage("[PCI] No ECAM regions found in MCFG table.\n");
        return new PortConfigSpace{};
    }

    KmDebugMessage("[PCI] ", mcfg->allocationCount(), " ECAM regions found in MCFG table.\n");
    return new McfgConfigSpace{mcfg, memory};
}
