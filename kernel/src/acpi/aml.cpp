#include "acpi/aml.hpp"

#include "kernel.hpp"

acpi::AmlParser::AmlParser(const RsdtHeader *header)
    : mCode((const uint8_t*)header + sizeof(RsdtHeader), header->length - sizeof(RsdtHeader))
{ }

uint8_t acpi::AmlParser::read() {
    if (mOffset >= mCode.size()) {
        return 0;
    }

    return mCode[mOffset++];
}

void acpi::WalkAml(const RsdtHeader *ssdt) {
    AmlParser parser(ssdt);

    uint8_t op = parser.read();
    KmDebugMessage("ACPI AML: ", km::Hex(op), "\n");
}
