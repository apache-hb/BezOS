#pragma once

#include "acpi/header.hpp"

#include "arena.hpp"

#include "std/static_vector.hpp"

#include <span>

#include <stdint.h>

namespace acpi {
    struct AmlName {
        using Segment = stdx::StaticString<4>;
        size_t prefix = 0;
        stdx::StaticVector<Segment, 8> segments;
    };

    struct AmlConst {
        enum class Type {
            eByte,
            eWord,
            eDword,
            eQword,
        } type;

        uint64_t value;
    };

    class AmlParser {
        uint32_t mOffset = 0;
        std::span<const uint8_t> mCode;

    public:
        AmlParser(std::span<const uint8_t> code)
            : mCode(code)
        { }

        AmlParser(const RsdtHeader *header);

        void advance() { mOffset += 1; }
        uint32_t offset() const { return mOffset; }

        uint8_t peek() const;
        uint8_t read();

        bool consume(uint8_t v) {
            if (peek() == v) {
                advance();
                return true;
            }

            return false;
        }
    };

    AmlName ReadAmlNameString(AmlParser& parser);

    struct AmlTermObject {

    };

    struct AmlTermArg {

    };

    struct AmlDefineName : AmlTermObject {
        stdx::StringView name;
    };

    // term arguments

    struct AmlLocalObject : AmlTermArg {
        enum class Type {
            eLocal0 = 0x60,
            eLocal1 = 0x61,
            eLocal2 = 0x62,
            eLocal3 = 0x63,
            eLocal4 = 0x64,
            eLocal5 = 0x65,
            eLocal6 = 0x66,
            eLocal7 = 0x67,
        };
    };

    // term objects

    struct AmlObject : AmlTermObject {

    };

    struct AmlNamedObject : AmlObject {

    };

    struct AmlDefCreateField : AmlNamedObject {

    };

    struct AmlCode {
        RsdtHeader header;
        km::Arena *arena;
    };

    AmlCode WalkAml(const RsdtHeader *dsdt, km::Arena *arena);
}
