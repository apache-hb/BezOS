#pragma once

#include "acpi/header.hpp"

#include "util/signature.hpp"
#include "common/util/util.hpp"

#include "util/format.hpp"

namespace acpi {
    enum class MadtFlags : uint32_t {
        ePcatCompat = (1 << 0),
    };

    UTIL_BITFLAGS(MadtFlags);

    enum class MadtEntryType : uint8_t {
        eLocalApic = 0,
        eIoApic = 1,
        eInterruptSourceOverride = 2,
        eNmiSource = 3,
        eLocalApicNmi = 4,
    };

    struct [[gnu::packed]] MadtEntry {
        struct [[gnu::packed]] LocalApic {
            uint8_t processorId;
            uint8_t apicId;
            uint32_t flags;

            bool isEnabled() const {
                static constexpr uint32_t kEnabled = 1 << 0;
                return flags & kEnabled;
            }

            bool isOnlineCapable() const {
                static constexpr uint32_t kOnlineCapable = 1 << 1;
                return flags & kOnlineCapable;
            }
        };

        struct [[gnu::packed]] IoApic {
            uint8_t ioApicId;
            uint8_t reserved;
            uint32_t address;
            uint32_t interruptBase;
        };

        struct [[gnu::packed]] InterruptSourceOverride {
            uint8_t bus;
            uint8_t source;
            uint32_t interrupt;
            uint16_t flags;
        };

        struct [[gnu::packed]] NonMaskableInterruptSource {
            uint32_t interrupt;
        };

        struct [[gnu::packed]] LocalApicNmi {
            uint8_t processorId;
            uint16_t flags;
            uint8_t lint;
        };

        MadtEntryType type;
        uint8_t length;

        union {
            LocalApic apic;
            IoApic ioapic;
            InterruptSourceOverride iso;
            NonMaskableInterruptSource nmiSource;
            LocalApicNmi apicNmi;
        };
    };

    class MadtIterator {
        const uint8_t *mCurrent;

    public:
        MadtIterator(const uint8_t *current)
            : mCurrent(current)
        { }

        MadtIterator& operator++();

        const MadtEntry *operator*();

        const MadtEntry *load() const noexcept {
            return reinterpret_cast<const MadtEntry*>(mCurrent);
        }

        friend bool operator!=(const MadtIterator& lhs, const MadtIterator& rhs);
    };

    struct [[gnu::packed]] Madt {
        static constexpr TableSignature kSignature = util::Signature("APIC");

        RsdtHeader header; // signature must be "APIC"

        uint32_t localApicAddress;
        MadtFlags flags;

        // these entries are variable length so i cant use a struct
        uint8_t entries[];

        MadtIterator begin() const {
            return MadtIterator { entries };
        }

        MadtIterator end() const {
            return MadtIterator { entries + header.length - sizeof(Madt) };
        }

        uint32_t ioApicCount() const;
        uint32_t lapicCount() const;
    };
}

template<>
struct km::Format<acpi::MadtEntryType> {
    static constexpr size_t kStringSize = km::Format<uint8_t>::kStringSize;
    static stdx::StringView toString(char *buffer, acpi::MadtEntryType type) {
        switch (type) {
        case acpi::MadtEntryType::eLocalApic:
            return "Local APIC";
        case acpi::MadtEntryType::eIoApic:
            return "IO APIC";
        case acpi::MadtEntryType::eInterruptSourceOverride:
            return "Interrupt Source Override";
        case acpi::MadtEntryType::eNmiSource:
            return "NMI Source";
        case acpi::MadtEntryType::eLocalApicNmi:
            return "Local APIC NMI";
        default:
            return km::Format<uint8_t>::toString(buffer, static_cast<uint8_t>(type));
        }
    }
};

template<>
struct std::iterator_traits<acpi::MadtIterator> {
    using iterator_category = std::forward_iterator_tag;
    using value_type = const acpi::MadtEntry*;
    using difference_type = std::ptrdiff_t;
    using pointer = const acpi::MadtEntry*;
    using reference = const acpi::MadtEntry*;
};
