#pragma once

#include "memory.hpp"

#include "acpi/mcfg.hpp"

#include <stdint.h>

#include <memory>

namespace pci {
    struct DeviceBusAddress {
        uint8_t bus;
        uint8_t slot;
        uint8_t function;
    };

    class IConfigSpace {
    public:
        void operator delete(IConfigSpace*, std::destroying_delete_t) {
            std::unreachable();
        }

        virtual ~IConfigSpace() = default;

        virtual uint32_t read32(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset) = 0;

        uint16_t read16(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset);
        uint8_t read8(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset);
    };

    struct EcamRegion {
        uint8_t first;
        uint8_t last;

        volatile void *base;

        bool contains(uint8_t bus) const {
            return first <= bus && last >= bus;
        }
    };

    class McfgConfigSpace final : public IConfigSpace {
        const acpi::Mcfg *mMcfg;
        std::unique_ptr<EcamRegion[]> mRegions;
    public:
        McfgConfigSpace(const acpi::Mcfg *mcfg, km::SystemMemory& memory);

        uint32_t read32(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset) override;
    };

    class PortConfigSpace final : public IConfigSpace {
    public:
        uint32_t read32(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset) override;
    };

    IConfigSpace *InitConfigSpace(const acpi::Mcfg *mcfg, km::SystemMemory& memory);
}
