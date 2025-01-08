#include "acpi/aml.hpp"

#include "kernel.hpp"

acpi::AmlParser::AmlParser(const RsdtHeader *header)
    : mCode((const uint8_t*)header + sizeof(RsdtHeader), header->length - sizeof(RsdtHeader))
{ }

uint8_t acpi::AmlParser::peek() const {
    if (mOffset >= mCode.size()) {
        return 0;
    }

    return mCode[mOffset];
}

uint8_t acpi::AmlParser::read() {
    if (mOffset >= mCode.size()) {
        return 0;
    }

    return mCode[mOffset++];
}

enum {
    kNameOp = 0x08,

    kDualNamePrefix = 0x2e,
    kMultiNamePrefix = 0x2f,
    kRootChar = 0x5c,
    kPrefixChar = 0x5e,
    kNullName = 0x00,
};

static bool IsLeadNameChar(uint8_t c) {
    // A-Z or _
    return (c >= 0x41 && c <= 0x5a) || c == 0x5f;
}

static bool IsNameChar(uint8_t c) {
    // A-Z, 0-9, or _
    return (c >= 0x41 && c <= 0x5a) || (c >= 0x30 && c <= 0x39) || c == 0x5f;
}

static uint8_t GetNameChar(acpi::AmlParser& parser) {
    uint8_t next = parser.read();
    if (!IsNameChar(next)) {
        KmDebugMessage("[AML] Invalid name character ", km::Hex(next).pad(2, '0'), " - ", parser.offset(), "\n");
    }

    return next;
}

static void NameSegBody(acpi::AmlParser& parser, acpi::AmlName& dst, char lead) {
    if (!IsLeadNameChar(lead)) {
        KmDebugMessage("[AML] Invalid lead name character ", km::Hex((uint8_t)lead).pad(2, '0'), "\n");
    }

    char a = GetNameChar(parser);
    char b = GetNameChar(parser);
    char c = GetNameChar(parser);

    dst.segments.add({ lead, a, b, c });
}

static void NameSeg(acpi::AmlParser& parser, acpi::AmlName& dst) {
    char lead = parser.read();
    NameSegBody(parser, dst, lead);
}

static void NamePath(acpi::AmlParser& parser, acpi::AmlName& dst) {
    uint8_t it = parser.read();
    switch (it) {
    case kMultiNamePrefix: {
        uint8_t count = parser.read();
        for (uint8_t i = 0; i < count; i++) {
            NameSeg(parser, dst);
        }
        break;
    }
    case kDualNamePrefix:
        NameSeg(parser, dst);
        NameSeg(parser, dst);
        break;

    case kNullName:
        break;

    default:
        NameSegBody(parser, dst, it);
        break;
    }
}

static void PrefixPath(acpi::AmlParser& parser, acpi::AmlName& name) {
    while (parser.consume(kPrefixChar)) {
        name.prefix += 1;
    }
}

acpi::AmlName acpi::ReadAmlNameString(AmlParser& parser) {
    acpi::AmlName name;

    uint8_t letter = parser.peek();
    switch (letter) {
    case kRootChar:
        NamePath(parser, name);
        break;

    default:
        PrefixPath(parser, name);
        NamePath(parser, name);
        break;
    }

    return { name };
}

static uint8_t ByteData(acpi::AmlParser& parser) {
    return parser.read();
}

static uint16_t WordData(acpi::AmlParser& parser) {
    return uint16_t(parser.read())
        | uint16_t(parser.read() << 8);
}

static uint32_t DwordData(acpi::AmlParser& parser) {
    return uint32_t(parser.read())
        | (uint32_t(parser.read()) << 8)
        | (uint32_t(parser.read()) << 16)
        | (uint32_t(parser.read()) << 24);
}

static uint64_t QwordData(acpi::AmlParser& parser) {
    return uint64_t(parser.read())
        | (uint64_t(parser.read()) << 8)
        | (uint64_t(parser.read()) << 16)
        | (uint64_t(parser.read()) << 24)
        | (uint64_t(parser.read()) << 32)
        | (uint64_t(parser.read()) << 40)
        | (uint64_t(parser.read()) << 48)
        | (uint64_t(parser.read()) << 56);
}

static acpi::AmlConst DataRefObject(acpi::AmlParser& parser) {
    enum {
        kBytePrefix = 0x0A,
        kWordPrefix = 0x0B,
        kDwordPrefix = 0x0C,
        kQwordPrefix = 0x0E,
    };

    uint8_t it = parser.read();

    switch (it) {
    case kBytePrefix:
        return { acpi::AmlConst::Type::eByte, ByteData(parser) };
    case kWordPrefix:
        return { acpi::AmlConst::Type::eWord, WordData(parser) };
    case kDwordPrefix:
        return { acpi::AmlConst::Type::eDword, DwordData(parser) };
    case kQwordPrefix:
        return { acpi::AmlConst::Type::eQword, QwordData(parser) };

    default:
        KmDebugMessage("[AML] Unknown DataRefObject: ", km::Hex(it).pad(2, '0'), "\n");
        return { acpi::AmlConst::Type::eByte, 0 };
    }
}

static void TermObj(acpi::AmlParser& parser, acpi::AmlCode&) {
    switch (parser.read()) {
    case kNameOp: {
        acpi::AmlName name = acpi::ReadAmlNameString(parser);
        acpi::AmlConst data = DataRefObject(parser);

        KmDebugMessage("[AML] Name: [", name.segments.count(), ":", name.prefix, "] ");
        for (size_t i = 0; i < name.segments.count(); i++) {
            if (i != 0) KmDebugMessage(".");
            KmDebugMessage(name.segments[i]);
        }
        KmDebugMessage(" = ", data.value, "\n");
        break;
    }
    }
}

acpi::AmlCode acpi::WalkAml(const RsdtHeader *dsdt, km::Arena *arena) {
    AmlParser parser(dsdt);

    AmlCode code = { *dsdt, arena };

    TermObj(parser, code);

    return code;
}
