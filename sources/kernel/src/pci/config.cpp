#include "pci/config.hpp"

#include "delay.hpp"
#include "logger/categories.hpp"
#include "memory/address_space.hpp"

using namespace stdx::literals;

static constexpr uint8_t kOffsetMask = 0b1111'1100;

static constexpr uint32_t kEnableBit = (1 << 31);

static constexpr uint16_t kAddressPort = 0xCF8;
static constexpr uint16_t kDataPort = 0xCFC;

static constexpr size_t kEcamSize = (256 * 32 * 8) * 0x1000;

static constexpr uint16_t ExtractWord(uint32_t data, uint16_t offset) {
    return (data >> ((offset & 2) * 8)) & 0xFFFF;
}

static constexpr uint8_t ExtractByte(uint32_t data, uint16_t offset) {
    uint32_t shift = (offset % 4) * 8;
    return (data >> shift) & 0xFF;
}

static_assert(ExtractWord(0xAABBCCDD, 0) == 0xCCDD);
static_assert(ExtractWord(0xAABBCCDD, 2) == 0xAABB);

static_assert(ExtractByte(0xAABBCCDD, 3) == 0xAA);
static_assert(ExtractByte(0xAABBCCDD, 2) == 0xBB);
static_assert(ExtractByte(0xAABBCCDD, 1) == 0xCC);
static_assert(ExtractByte(0xAABBCCDD, 0) == 0xDD);

// generic PCI configuration space read

uint16_t pci::IConfigSpace::read16(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset) {
    uint32_t data = read32(bus, slot, function, offset & ~0b11);
    return ExtractWord(data, offset);
}

uint8_t pci::IConfigSpace::read8(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset) {
    uint32_t data = read32(bus, slot, function, offset & ~0b11);
    return ExtractByte(data, offset);
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

OsStatus pci::PortConfigSpace::create(PortConfigSpace **space [[gnu::nonnull]]) [[clang::allocating]] {
    void *ptr = aligned_alloc(alignof(PortConfigSpace), sizeof(PortConfigSpace));
    if (ptr == nullptr) {
        return OsStatusOutOfMemory;
    }

    *space = new (ptr) PortConfigSpace();
    return OsStatusSuccess;
}

// MMIO-based PCI configuration space read

OsStatus pci::McfgConfigSpace::create(const acpi::Mcfg *mcfg, km::AddressSpace& memory, McfgConfigSpace **space [[gnu::nonnull]]) [[clang::allocating]] {
    void *ptr = aligned_alloc(alignof(McfgConfigSpace), sizeof(McfgConfigSpace) + (sizeof(EcamRegion) * mcfg->allocationCount()));
    if (ptr == nullptr) {
        return OsStatusOutOfMemory;
    }

    McfgConfigSpace *result = new (ptr) McfgConfigSpace(mcfg);

    for (size_t i = 0; i < mcfg->allocationCount(); i++) {
        const acpi::McfgAllocation& allocation = mcfg->allocations[i];

        km::MemoryRangeEx range = km::MemoryRangeEx::of(allocation.address, kEcamSize);

        km::VmemAllocation mapping;

        if (OsStatus status = memory.map(range, km::PageFlags::eData, km::MemoryType::eUncached, &mapping)) {
            PciLog.fatalf("Failed to map ECAM region ", range);
            for (size_t j = 0; j < i; j++) {
                (void)memory.unmap(result->regionAt(j).mapping);
            }
            free(result);
            return status;
        }

        EcamRegion region = {
            .mapping = mapping,
            .first = allocation.startBusNumber,
            .last = allocation.endBusNumber
        };

        result->regionAt(i) = region;

        auto ecam = km::VirtualRangeEx::of((void*)region.baseAddress(), kEcamSize);

        PciLog.infof("ECAM region ", i, ": ", range, " -> ", ecam);
    }

    *space = result;
    return OsStatusSuccess;
}

uint32_t pci::McfgConfigSpace::read32(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset) {
    for (size_t i = 0; i < mMcfg->allocationCount(); i++) {
        const EcamRegion& region = mRegions[i];
        if (!region.contains(bus)) continue;

        uint32_t address = (uint32_t(bus) << 20) | (uint32_t(slot) << 15) | (uint32_t(function) << 12) | offset;

        return *(uint32_t*)((uint8_t*)region.baseAddress() + address);
    }

    PciLog.warnf("ECAM region not found for ", km::Hex(bus).pad(2), ":", km::Hex(slot).pad(2), ":", km::Hex(function).pad(2));
    return UINT32_MAX;
}

pci::IConfigSpace *pci::setupConfigSpace(const acpi::Mcfg *mcfg, km::AddressSpace& memory) {
    IConfigSpace *space;
    if (OsStatus status = setupConfigSpace(mcfg, memory, &space)) {
        PciLog.fatalf("Failed to setup PCI config space: ", OsStatusId(status));
        return nullptr;
    }
    return space;
}

OsStatus pci::setupConfigSpace(const acpi::Mcfg *mcfg, km::AddressSpace& memory, IConfigSpace **space) [[clang::allocating]] {
    if (mcfg == nullptr) {
        PciLog.warnf("No MCFG table.");
        return PortConfigSpace::create(std::bit_cast<PortConfigSpace**>(space));
    }

    size_t count = mcfg->allocationCount();
    if (count == 0) {
        PciLog.warnf("No ECAM regions found in MCFG table.");
        return PortConfigSpace::create(std::bit_cast<PortConfigSpace**>(space));
    }

    PciLog.dbgf(count, " ECAM ", (count == 1) ? "region"_sv : "regions"_sv, " found in MCFG table.");
    return McfgConfigSpace::create(mcfg, memory, std::bit_cast<McfgConfigSpace**>(space));
}
