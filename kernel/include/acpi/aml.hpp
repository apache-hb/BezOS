#pragma once

#include "acpi/header.hpp"
#include <span>

#include <stdint.h>

namespace acpi {
    using AmlByte = uint8_t;
    using AmlWord = uint16_t;
    using AmlDword = uint32_t;
    using AmlQword = uint64_t;

    class AmlParser {
        uint32_t mOffset = 0;
        std::span<const uint8_t> mCode;

    public:
        AmlParser(std::span<const uint8_t> code)
            : mCode(code)
        { }

        AmlParser(const RsdtHeader *header);

        AmlByte amlByteData();
        AmlWord amlWordData();
        AmlDword amlDwordData();
        AmlQword amlQwordData();

        uint8_t peek() const;
        uint8_t read();
    };

    struct AmlTermObject {

    };

    struct AmlTermArg {

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
    };

    void WalkAml(const RsdtHeader *ssdt);
}
