#include "acpi/aml.hpp"

#include "kernel.hpp"

using namespace stdx::literals;

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
    enum {
        kDualNamePrefix = 0x2e,
        kMultiNamePrefix = 0x2f,
        kNullName = 0x00,
    };

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

enum {
    kPrefixChar = 0x5e,
    kRootChar = 0x5c,
};

static void PrefixPath(acpi::AmlParser& parser, acpi::AmlName& name) {
    while (parser.consume(kPrefixChar)) {
        name.prefix += 1;
    }
}

acpi::AmlName NameString(acpi::AmlParser& parser) {
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

static stdx::StringView StringData(acpi::AmlParser& parser) {
    const char *first = parser.current();
    while (parser.peek() != 0) {
        parser.advance();
    }
    const char *last = parser.current();

    parser.consume('\0');

    return { first, last };
}

static uint32_t PkgLength(acpi::AmlParser& parser) {
    uint8_t lead = parser.read();
    uint8_t extra = (lead & 0b1100'0000) >> 6;

    uint32_t value = 0;
    if (extra > 0) {
        value = lead & 0b0000'1111;

        for (uint8_t i = 0; i < extra; i++) {
            value |= (uint32_t(parser.read()) << (((i + 1) * 8) - 4));
        }
    } else {
        value = lead & 0b0011'1111;
    }

    return value;
}

static acpi::AmlData DataRefObject(acpi::AmlParser& parser, acpi::AmlNodeBuffer& code);

static acpi::AmlPackageId DefPackage(acpi::AmlParser& parser, acpi::AmlNodeBuffer& code) {
    uint32_t length = PkgLength(parser);
    (void)length; // TODO: Should probably limit parsing with length as well as count

    uint8_t count = ByteData(parser);

    acpi::AmlAllocator *alloc = code.allocator();

    auto deleter = [&](acpi::AmlData *ptr) {
        alloc->deallocateArray(ptr, count);
    };

    std::unique_ptr<acpi::AmlData[], decltype(deleter)> terms{alloc->allocateArray<acpi::AmlData>(count), deleter};

    for (uint8_t i = 0; i < count; i++) {
        terms[i] = DataRefObject(parser, code);
    }

    acpi::AmlIdRange elements = code.addRange([&] {
        for (uint8_t i = 0; i < count; i++) {
            code.add(acpi::AmlDataTerm{ terms[i] });
        }
    });

    return code.add(acpi::AmlPackageTerm { elements });
}

static acpi::AmlData DataRefObject(acpi::AmlParser& parser, acpi::AmlNodeBuffer& code) {
    enum {
        kBytePrefix = 0x0A,
        kWordPrefix = 0x0B,
        kDwordPrefix = 0x0C,
        kQwordPrefix = 0x0E,
        kStringPrefix = 0x0D,

        kPackageOp = 0x12,

        kZeroOp = 0x00,
        kOneOp = 0x01,
        kOnesOp = 0xFF,
    };

    uint8_t it = parser.read();

    switch (it) {
    case kBytePrefix:
        return { acpi::AmlData::Type::eByte, { ByteData(parser) } };
    case kWordPrefix:
        return { acpi::AmlData::Type::eWord, { WordData(parser) } };
    case kDwordPrefix:
        return { acpi::AmlData::Type::eDword, { DwordData(parser) } };
    case kQwordPrefix:
        return { acpi::AmlData::Type::eQword, { QwordData(parser) } };
    case 'A'...'Z': case '_': {
        acpi::AmlName name;
        NameSegBody(parser, name, it);
        return { acpi::AmlData::Type::eString, { name.segments[0] } };
    }
    case kStringPrefix:
        parser.advance();
        return { acpi::AmlData::Type::eString, { StringData(parser) } };
    case kPackageOp:
        return { acpi::AmlData::Type::ePackage, { DefPackage(parser, code) } };
    case kZeroOp:
        return { acpi::AmlData::Type::eByte, { 0 } };
    case kOneOp:
        return { acpi::AmlData::Type::eByte, { 1 } };
    case kOnesOp:
        return { acpi::AmlData::Type::eQword, { 0xFFFF'FFFF'FFFF'FFFF } };
    default:
        KmDebugMessage("[AML] Unknown DataRefObject: ", km::Hex(it).pad(2, '0'), "\n");
        return { acpi::AmlData::Type::eByte, { 0 } };
    }
}

struct AmlMethodFlags {
    uint8_t flags;

    uint8_t argCount() const {
        return flags & 0b0000'0111;
    }

    bool serialized() const {
        return flags & 0b0000'1000;
    }

    uint8_t syncLevel() const {
        return (flags & 0b1111'0000) >> 4;
    }
};

static AmlMethodFlags MethodFlags(acpi::AmlParser& parser) {
    uint8_t flags = parser.read();
    return { flags };
}

struct AmlFieldFlags {
    uint8_t flags;

    bool lock() const {
        return flags & (1 << 4);
    }
};

static AmlFieldFlags FieldFlags(acpi::AmlParser& parser) {
    uint8_t flags = parser.read();
    return { flags };
}

static void FieldList(acpi::AmlParser& parser, uint32_t length) {
    enum {
        kReservedField = 0x00,
        kAccessField = 0x01,
    };

    uint32_t end = parser.offset() + length;

    KmDebugMessage("[AML] Reading until ", km::Hex(end).pad(8, '0'), "\n");

    while (parser.offset() < end) {
        uint8_t it = parser.peek();
        switch (it) {
        case kReservedField: {
            parser.advance();
            uint32_t length = PkgLength(parser);
            KmDebugMessage("[AML] ReservedField length ", length, "\n");
            break;
        }
        case kAccessField: {
            parser.advance();
            ByteData(parser);
            ByteData(parser);
            break;
        }
        default:
            KmDebugMessage("[AML] Unknown Field ", km::Hex(it).pad(2, '0'), "\n");
            return;
        }
    }
}

static void TermObj(acpi::AmlParser& parser, acpi::AmlNodeBuffer& code) {
    enum {
        kNameOp = 0x08,
        kScopeOp = 0x10,
        kMethodOp = 0x14,
        kExtOpPrefix = 0x5B,

        kRegionOp = 0x80,
        kFieldOp = 0x81,
        kIndexFieldOp = 0x86,
    };

    switch (uint8_t op = parser.read()) {
    case kNameOp: {
        acpi::AmlName name = NameString(parser);
        acpi::AmlData data = DataRefObject(parser, code);

        KmDebugMessage("[AML] Name (", name, ", ", data, ")\n");
        break;
    }
    case kScopeOp: {
        uint32_t front = parser.offset();

        uint32_t length = PkgLength(parser);
        acpi::AmlName name = NameString(parser);

        uint32_t back = parser.offset();

        KmDebugMessage("[AML] Scope (", name, ") - ", length, "\n");

        break;
    }
    case kMethodOp: {
        uint32_t front = parser.offset();

        uint32_t length = PkgLength(parser);
        acpi::AmlName name = NameString(parser);
        AmlMethodFlags flags = MethodFlags(parser);

        uint32_t back = parser.offset();

        KmDebugMessage("[AML] Method (", name, ", ", flags.argCount(), ", ", flags.serialized() ? "Serialized"_sv : "NotSerialized"_sv, ")\n");

        parser.skip(length - (back - front));
        break;
    }
    case kExtOpPrefix: {
        uint8_t ext = parser.read();
        switch (ext) {
        case kRegionOp: {
            acpi::AmlName name = NameString(parser);
            acpi::AddressSpaceId region = acpi::AddressSpaceId(ByteData(parser));
            acpi::AmlData offset = DataRefObject(parser, code);
            acpi::AmlData size = DataRefObject(parser, code);

            KmDebugMessage("[AML] OperationRegion (", name, ", ", region, ", ", offset, ", ", size, ")\n");

            acpi::AmlOpRegionTerm opRegion = { name, region, offset, size };
            code.add(opRegion);
            break;
        }
        case kFieldOp: {
            uint32_t front = parser.offset();

            uint32_t length = PkgLength(parser);
            acpi::AmlName name = NameString(parser);
            AmlFieldFlags flags = FieldFlags(parser);

            uint32_t back = parser.offset();

            KmDebugMessage("[AML] Field (", name, ", ", flags.lock() ? "Lock"_sv : "NoLock"_sv, ")\n");

            parser.skip(length - (back - front));
            break;
        }
        case kIndexFieldOp: {
            uint32_t front = parser.offset();

            uint32_t length = PkgLength(parser);
            acpi::AmlName name = NameString(parser);
            acpi::AmlName field = NameString(parser);
            AmlFieldFlags flags = FieldFlags(parser);

            // FieldList(parser, length);

            uint32_t back = parser.offset();

            KmDebugMessage("[AML] IndexField length ", length, " - ", back - front, "\n");

            KmDebugMessage("[AML] IndexField (", name, ", ", field, ", ", flags.lock() ? "Lock"_sv : "NoLock"_sv, ")\n");

            // -2 for the prefix
            parser.skip(length - (back - front));
            break;
        }
        default:
            KmDebugMessage("[AML] Unknown ExtendedOp ", km::Hex(ext).pad(2, '0'), "\n");
            break;
        }
        break;
    }
    default:
        KmDebugMessage("[AML] Unknown TermObj ", km::Hex(op).pad(2, '0'), "\n");
        break;
    }
}

acpi::AmlCode acpi::WalkAml(const RsdtHeader *dsdt, AmlAllocator arena) {
    AmlCode code = { *dsdt, arena };

    AmlParser parser(dsdt);

    while (!parser.done()) {
        TermObj(parser, code.nodes());
    }

    return code;
}

acpi::AmlNameId acpi::AmlNodeBuffer::add(AmlNameTerm term) {
    return addTerm(term, AmlTermType::eName);
}

acpi::AmlMethodId acpi::AmlNodeBuffer::add(AmlMethodTerm term) {
    return addTerm(term, AmlTermType::eMethod);
}

acpi::AmlPackageId acpi::AmlNodeBuffer::add(AmlPackageTerm term) {
    return addTerm(term, AmlTermType::ePackage);
}

acpi::AmlDataId acpi::AmlNodeBuffer::add(AmlDataTerm term) {
    return addTerm(term, AmlTermType::eData);
}

acpi::AmlOpRegionId acpi::AmlNodeBuffer::add(AmlOpRegionTerm term) {
    return addTerm(term, AmlTermType::eOpRegion);
}

using AmlNameFormat = km::Format<acpi::AmlName>;
using AmlConstFormat = km::Format<acpi::AmlData>;

void AmlNameFormat::format(km::IOutStream& out, const acpi::AmlName& value) {
    for (size_t i = 0; i < value.prefix; i++) {
        out.write('\\');
    }
    for (size_t i = 0; i < value.segments.count(); i++) {
        if (i != 0) out.write('.');
        out.write(value.segments[i]);
    }
}

void AmlConstFormat::format(km::IOutStream& out, const acpi::AmlData& value) {
    switch (value.type) {
    case acpi::AmlData::Type::eByte:
        out.format(km::Hex(value.data.integer).pad(2));
        break;
    case acpi::AmlData::Type::eWord:
        out.format(km::Hex(value.data.integer).pad(4));
        break;
    case acpi::AmlData::Type::eDword:
        out.format(km::Hex(value.data.integer).pad(8));
        break;
    case acpi::AmlData::Type::eQword:
        out.format(km::Hex(value.data.integer).pad(16));
        break;
    case acpi::AmlData::Type::eString:
        out.format("\"", value.data.string, "\"");
        break;
    case acpi::AmlData::Type::ePackage:
        out.write("package");
        break;
    default:
        out.write("unknown");
    }
}
