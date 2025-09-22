#pragma once

#include "acpi/mcfg.hpp"
#include "memory/heap.hpp"
#include "memory/vmm_heap.hpp"

#include <stdint.h>

namespace km {
    class AddressSpace;
}

namespace pci {
    struct DeviceBusAddress {
        uint8_t bus;
        uint8_t slot;
        uint8_t function;
    };

    enum class ConfigSpaceType {
        eMcfg,
        ePort,
    };

    class IConfigSpace {
    public:
        virtual ~IConfigSpace() = default;

        virtual ConfigSpaceType type() const = 0;
        virtual uint32_t read32(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset) = 0;

        uint16_t read16(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset);
        uint8_t read8(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset);
    };

    struct EcamRegion {
        km::VmemAllocation mapping;
        uint8_t first;
        uint8_t last;

        bool contains(uint8_t bus) const {
            return first <= bus && last >= bus;
        }

        volatile void *baseAddress() const {
            return std::bit_cast<void*>(mapping.address());
        }
    };

    class McfgConfigSpace final : public IConfigSpace {
        const acpi::Mcfg *mMcfg;
        EcamRegion mRegions[];

        EcamRegion &regionAt(uint8_t index) {
            return mRegions[index];
        }

        McfgConfigSpace(const acpi::Mcfg *mcfg)
            : mMcfg(mcfg)
        { }

    public:
        McfgConfigSpace() = default;

        ConfigSpaceType type() const override { return ConfigSpaceType::eMcfg; }
        uint32_t read32(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset) override;

        [[nodiscard]]
        static OsStatus create(const acpi::Mcfg *mcfg, km::AddressSpace& memory, McfgConfigSpace **space [[gnu::nonnull]]) [[clang::allocating]];
    };

    class PortConfigSpace final : public IConfigSpace {
    public:
        ConfigSpaceType type() const override { return ConfigSpaceType::ePort; }
        uint32_t read32(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset) override;

        [[nodiscard]]
        static OsStatus create(PortConfigSpace **space [[gnu::nonnull]]) [[clang::allocating]];
    };

    IConfigSpace *setupConfigSpace(const acpi::Mcfg *mcfg, km::AddressSpace& memory);

    [[nodiscard]]
    OsStatus setupConfigSpace(const acpi::Mcfg *mcfg, km::AddressSpace& memory, IConfigSpace **space) [[clang::allocating]];
}
