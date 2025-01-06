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

        AmlByte amlByteData();
        AmlWord amlWordData();
        AmlDword amlDwordData();
        AmlQword amlQwordData();

        uint8_t peek() const;
        uint8_t read();
    };

    struct AmlObject {

    };

    struct AmlCreateField : public AmlObject {

    };

    struct AmlCode {
        RsdtHeader header;
    };
}
