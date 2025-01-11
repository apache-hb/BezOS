#include "acpi/aml.hpp"

#include "kernel.hpp"

using namespace stdx::literals;
using namespace acpi::detail;

acpi::AmlParser::AmlParser(const RsdtHeader *header)
    : AmlParser(std::span((const uint8_t*)header + sizeof(RsdtHeader), header->length - sizeof(RsdtHeader)))
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

enum {
    kPrefixChar = 0x5e,
    kRootChar = 0x5c,
};

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

static void PrefixPath(acpi::AmlParser& parser, acpi::AmlName& name) {
    while (parser.consume(kPrefixChar)) {
        name.prefix += 1;
    }
}

acpi::AmlName acpi::detail::NameString(acpi::AmlParser& parser) {
    acpi::AmlName name = { 0 };

    uint8_t letter = parser.peek();
    switch (letter) {
    case kRootChar:
        parser.advance();
        name.prefix = SIZE_MAX;
        NamePath(parser, name);
        break;

    default:
        PrefixPath(parser, name);
        NamePath(parser, name);
        break;
    }

    return { name };
}

uint8_t acpi::detail::ByteData(acpi::AmlParser& parser) {
    return parser.read();
}

uint16_t acpi::detail::WordData(acpi::AmlParser& parser) {
    return uint16_t(parser.read())
        | uint16_t(parser.read() << 8);
}

uint32_t acpi::detail::DwordData(acpi::AmlParser& parser) {
    return uint32_t(parser.read())
        | (uint32_t(parser.read()) << 8)
        | (uint32_t(parser.read()) << 16)
        | (uint32_t(parser.read()) << 24);
}

uint64_t acpi::detail::QwordData(acpi::AmlParser& parser) {
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

acpi::AmlAnyId acpi::detail::TermArg(AmlParser& parser, acpi::AmlNodeBuffer& code) {
    // TODO: normalize the parsing
    if (parser.peek() == 0x5B) {
        return Term(parser, code);
    }

    return code.add(AmlDataTerm{DataRefObject(parser, code)});
}

acpi::AmlBuffer acpi::detail::BufferData(acpi::AmlParser& parser, acpi::AmlNodeBuffer& code) {
    size_t front = parser.offset();

    uint32_t length = PkgLength(parser);

    acpi::AmlAnyId size = TermArg(parser, code);
    size_t back = parser.offset();

    size_t space = length - (back - front);

    stdx::Vector<uint8_t> data{code.allocator(), length};
    for (uint32_t i = 0; i < space; i++) {
        data.add(parser.read());
    }

    return { size, data };
}

uint32_t acpi::detail::PkgLength(acpi::AmlParser& parser) {
    uint8_t lead = parser.read();

    if (uint8_t count = (lead & 0b1100'0000) >> 6) {
        uint32_t value = lead & 0b0000'1111;

        for (uint8_t i = 0; i < count; i++) {
            value |= (uint32_t(parser.read()) << (((i + 1) * 8) - 4));
        }
        return value;
    } else {
        return lead & 0b0011'1111;
    }
}

static acpi::AmlPackageId DefPackage(acpi::AmlParser& parser, acpi::AmlNodeBuffer& code) {
    uint32_t front = parser.offset();

    uint32_t length = PkgLength(parser);
    uint8_t count = ByteData(parser);

    uint32_t back = parser.offset();

    acpi::AmlParser sub = parser.subspan(length - (back - front));

    stdx::Vector<acpi::AmlAnyId> terms(code.allocator());

    for (uint8_t i = 0; i < count; i++) {
        terms.add(code.add(acpi::AmlDataTerm{DataRefObject(sub, code)}));
    }

    return code.add(acpi::AmlPackageTerm { terms });
}

enum {
    kBytePrefix = 0x0A,
    kWordPrefix = 0x0B,
    kDwordPrefix = 0x0C,
    kQwordPrefix = 0x0E,
    kStringPrefix = 0x0D,
    kBufferPrefix = 0x11,

    kPackageOp = 0x12,

    kZeroOp = 0x00,
    kOneOp = 0x01,
    kOnesOp = 0xFF,
};

acpi::AmlData acpi::detail::DataRefObject(acpi::AmlParser& parser, acpi::AmlNodeBuffer& code) {

    uint8_t it = parser.read();

    switch (it) {
    case 'A'...'Z': case '_': {
        acpi::AmlName name;
        NameSegBody(parser, name, it);
        return { acpi::AmlData::Type::eString, { name.segments[0] } };
    }
    case kBytePrefix:
        return { acpi::AmlData::Type::eByte, { ByteData(parser) } };
    case kWordPrefix:
        return { acpi::AmlData::Type::eWord, { WordData(parser) } };
    case kDwordPrefix:
        return { acpi::AmlData::Type::eDword, { DwordData(parser) } };
    case kQwordPrefix:
        return { acpi::AmlData::Type::eQword, { QwordData(parser) } };
    case kStringPrefix:
        parser.advance();
        return { acpi::AmlData::Type::eString, { StringData(parser) } };
    case kBufferPrefix:
        return { acpi::AmlData::Type::eBuffer, { BufferData(parser, code) } };
    case kPackageOp:
        return { acpi::AmlData::Type::ePackage, { DefPackage(parser, code) } };
    case kZeroOp:
        return { acpi::AmlData::Type::eByte, { 0ull } };
    case kOneOp:
        return { acpi::AmlData::Type::eByte, { 1ull } };
    case kOnesOp:
        return code.ones();
    default:
        KmDebugMessage("[AML] Unknown DataRefObject: ", km::Hex(it).pad(2, '0'), "\n");
        return { acpi::AmlData::Type::eByte, { 0ull } };
    }
}

static acpi::AmlMethodFlags MethodFlags(acpi::AmlParser& parser) {
    uint8_t flags = parser.read();
    return { flags };
}

static acpi::AmlFieldFlags FieldFlags(acpi::AmlParser& parser) {
    uint8_t flags = parser.read();
    return { flags };
}

acpi::AmlNameTerm acpi::detail::NameTerm(AmlParser& parser, AmlNodeBuffer& code) {
    acpi::AmlName name = NameString(parser);
    acpi::AmlData data = DataRefObject(parser, code);

    // KmDebugMessage("[AML] Name (", name, ", ", data, ")\n");

    return { name, code.add(AmlDataTerm(data)) };
}

acpi::AmlSuperName acpi::detail::SuperName(AmlParser& parser) {
    acpi::AmlName name = NameString(parser);
    return { name };
}

acpi::AmlTarget acpi::detail::Target(AmlParser& parser) {
    if (parser.consume('\0')) {
        return acpi::AmlTarget { };
    }

    acpi::AmlSuperName name = SuperName(parser);
    return { name };
}

acpi::AmlScopeTerm acpi::detail::ScopeTerm(AmlParser& parser, AmlNodeBuffer& code) {
    uint32_t front = parser.offset();

    uint32_t length = PkgLength(parser);
    acpi::AmlName name = NameString(parser);

    uint32_t back = parser.offset();

    stdx::Vector<acpi::AmlAnyId> terms(code.allocator());

    acpi::AmlParser sub = parser.subspan(length - (back - front));

    while (!sub.done()) {
        terms.add(Term(sub, code));
    }

    // KmDebugMessage("[AML] Scope (", name, ") - ", (length - (back - front)), "\n");

    return acpi::AmlScopeTerm { name, terms };
}

acpi::AmlMethodTerm acpi::detail::MethodTerm(AmlParser& parser, AmlNodeBuffer& code) {
    uint32_t front = parser.offset();

    uint32_t length = PkgLength(parser);
    acpi::AmlName name = NameString(parser);
    AmlMethodFlags flags = MethodFlags(parser);

    uint32_t back = parser.offset();

    acpi::AmlParser sub = parser.subspan(length - (back - front));

    // KmDebugMessage("[AML] Method (", name, ", ", flags.argCount(), ", ", flags.serialized() ? "Serialized"_sv : "NotSerialized"_sv, ")\n");

    return { name, flags };
}

acpi::AmlAliasTerm acpi::detail::AliasTerm(AmlParser& parser, AmlNodeBuffer& code) {
    acpi::AmlName name = NameString(parser);
    acpi::AmlName alias = NameString(parser);

    // KmDebugMessage("[AML] Alias (", name, ", ", alias, ")\n");

    return { name, alias };
}

acpi::AmlDeviceTerm acpi::detail::DeviceTerm(AmlParser& parser, AmlNodeBuffer& code) {
    uint32_t front = parser.offset();
    uint32_t length = PkgLength(parser);
    acpi::AmlName name = NameString(parser);
    uint32_t back = parser.offset();

    acpi::AmlParser sub = parser.subspan(length - (back - front));

    // KmDebugMessage("[AML] Device (", name, ") - ", (length - (back - front)), "\n");

    return { name };
}

acpi::AmlProcessorTerm acpi::detail::ProcessorTerm(AmlParser& parser, AmlNodeBuffer& code) {
    uint32_t front = parser.offset();
    uint32_t length = PkgLength(parser);
    acpi::AmlName name = NameString(parser);
    uint8_t id = ByteData(parser);
    uint32_t pblkAddr = DwordData(parser);
    uint8_t pblkLen = ByteData(parser);

    uint32_t back = parser.offset();

    acpi::AmlParser sub = parser.subspan(length - (back - front));

    // KmDebugMessage("[AML] Processor (", name, ", ", id, ") - ", (length - (back - front)), "\n");

    return { name, id, pblkAddr, pblkLen };
}

acpi::AmlIfElseOp acpi::detail::DefIfElse(AmlParser& parser, AmlNodeBuffer& code) {
    uint32_t front = parser.offset();

    uint32_t length = PkgLength(parser);

    AmlAnyId predicate = TermArg(parser, code);

    uint32_t back = parser.offset();

    acpi::AmlParser sub = parser.subspan(length - (back - front));

    stdx::Vector<acpi::AmlAnyId> terms(code.allocator());

    while (!sub.done()) {
        terms.add(Term(sub, code));
    }

    // KmDebugMessage("[AML] IfElse () - ", (length - (back - front)), "\n");

    return { predicate, terms };
}

acpi::AmlCondRefOp acpi::detail::ConfRefOf(AmlParser& parser, AmlNodeBuffer& code) {
    acpi::AmlSuperName name = SuperName(parser);
    acpi::AmlTarget target = Target(parser);

    // KmDebugMessage("[AML] CondRefOf (", name.name, ", ", target.name.name, ")\n");

    return { name, target };
}

acpi::AmlOpRegionTerm acpi::detail::DefOpRegion(AmlParser& parser, AmlNodeBuffer& code) {
    acpi::AmlName name = NameString(parser);
    acpi::AddressSpaceId region = acpi::AddressSpaceId(ByteData(parser));
    acpi::AmlAnyId offset = TermArg(parser, code);
    acpi::AmlAnyId size = TermArg(parser, code);

    // KmDebugMessage("[AML] OperationRegion (", name, ", ", region, ")\n");

    acpi::AmlOpRegionTerm opRegion = { name, region, offset, size };

    return opRegion;
}

acpi::AmlStoreTerm acpi::detail::DefStore(AmlParser& parser, AmlNodeBuffer& code) {
    acpi::AmlAnyId source = TermArg(parser, code);
    acpi::AmlSuperName target = SuperName(parser);

    // KmDebugMessage("[AML] Store (", target.name, ")\n");

    return { source, target };
}

acpi::CreateDwordFieldTerm acpi::detail::DefCreateDwordField(AmlParser& parser, AmlNodeBuffer& code) {
    AmlAnyId source = TermArg(parser, code);
    AmlAnyId index = TermArg(parser, code);
    acpi::AmlName name = NameString(parser);

    // KmDebugMessage("[AML] CreateDwordField (", name, ")\n");

    return { source, index, name };
}

acpi::AmlAnyId acpi::detail::Term(acpi::AmlParser& parser, acpi::AmlNodeBuffer& code) {
    enum {
        kNameOp = 0x08,
        kAliasOp = 0x06,
        kScopeOp = 0x10,
        kCondRefOp = 0x12,
        kMethodOp = 0x14,
        kStoreOp = 0x70,
        kIfOp = 0xA0,

        kCreateDwordFieldOp = 0x8A,

        kExtOpPrefix = 0x5B,

        kRegionOp = 0x80,
        kFieldOp = 0x81,
        kDeviceOp = 0x82,
        kProcessorOp = 0x83,
        kDerefOfOp = 0x83,
        kIndexFieldOp = 0x86,
    };

    switch (uint8_t op = parser.read()) {
    case kNameOp:
        return code.add(NameTerm(parser, code));

    case kAliasOp:
        return code.add(AliasTerm(parser, code));

    case kScopeOp:
        return code.add(ScopeTerm(parser, code));

    case kMethodOp:
        return code.add(MethodTerm(parser, code));

    case kIfOp:
        return code.add(DefIfElse(parser, code));

    case kStoreOp:
        return code.add(DefStore(parser, code));

    case kCreateDwordFieldOp:
        return code.add(DefCreateDwordField(parser, code));

    case kBytePrefix:
        return code.add(AmlDataTerm{ AmlData{ acpi::AmlData::Type::eByte, { ByteData(parser) } } });
    case kWordPrefix:
        return code.add(AmlDataTerm{ AmlData{ acpi::AmlData::Type::eWord, { WordData(parser) } }});
    case kDwordPrefix:
        return code.add(AmlDataTerm{ AmlData{ acpi::AmlData::Type::eDword, { DwordData(parser) } }});
    case kQwordPrefix:
        return code.add(AmlDataTerm{ AmlData{ acpi::AmlData::Type::eQword, { QwordData(parser) } }});
    case kStringPrefix:
        parser.advance();
        return code.add(AmlDataTerm{ AmlData{ acpi::AmlData::Type::eString, { StringData(parser) } }});
    case kBufferPrefix:
        return code.add(AmlDataTerm{ AmlData{ acpi::AmlData::Type::eBuffer, { BufferData(parser, code) } }});
    case kPackageOp:
        return code.add(AmlDataTerm{ AmlData{ acpi::AmlData::Type::ePackage, { DefPackage(parser, code) } }});
    case kZeroOp:
        return code.add(AmlDataTerm{ AmlData{ acpi::AmlData::Type::eByte, { 0ull } }});
    case kOneOp:
        return code.add(AmlDataTerm{ AmlData{ acpi::AmlData::Type::eByte, { 1ull } }});
    case kOnesOp:
        return code.add(AmlDataTerm{ code.ones() });

    case kExtOpPrefix: {
        uint8_t ext = parser.read();
        switch (ext) {
        case kRegionOp:
            return code.add(DefOpRegion(parser, code));

        case kFieldOp: {
            uint32_t front = parser.offset();

            uint32_t length = PkgLength(parser);
            acpi::AmlName name = NameString(parser);
            AmlFieldFlags flags = FieldFlags(parser);

            uint32_t back = parser.offset();

            // KmDebugMessage("[AML] Field (", name, ", ", flags.lock() ? "Lock"_sv : "NoLock"_sv, ")\n");

            parser.skip(length - (back - front));

            return code.add(AmlFieldTerm{ name, flags });
        }

        case kDeviceOp:
            return code.add(DeviceTerm(parser, code));

        case kProcessorOp:
            return code.add(ProcessorTerm(parser, code));

        case kCondRefOp:
            return code.add(ConfRefOf(parser, code));

        case kIndexFieldOp: {
            uint32_t front = parser.offset();

            uint32_t length = PkgLength(parser);
            acpi::AmlName name = NameString(parser);
            acpi::AmlName field = NameString(parser);
            AmlFieldFlags flags = FieldFlags(parser);

            // FieldList(parser, length);

            uint32_t back = parser.offset();

            // KmDebugMessage("[AML] IndexField length ", length, " - ", back - front, "\n");

            // KmDebugMessage("[AML] IndexField (", name, ", ", field, ", ", flags.lock() ? "Lock"_sv : "NoLock"_sv, ")\n");

            parser.skip(length - (back - front));

            return code.add(AmlIndexFieldTerm{ name, field, flags });
        }
        default:
            KmDebugMessage("[AML] Unknown ExtendedOp ", km::Hex(ext).pad(2, '0'), "\n");
            break;
        }
        break;
    }
    default:
        KmDebugMessage("[AML] Unknown TermObj ", km::Hex(op).pad(2, '0'), "\n");
        return code.add(AmlDataTerm{DataRefObject(parser, code)});
    }

    return { 0 };
}

acpi::AmlCode acpi::WalkAml(const RsdtHeader *dsdt, mem::IAllocator *arena) {
    AmlCode code = { *dsdt, arena };

    AmlParser parser(dsdt);

    stdx::Vector<AmlAnyId> terms(arena);

    while (!parser.done()) {
        terms.add(Term(parser, code.nodes()));
    }

    code.root()->terms = terms;

    return code;
}

acpi::AmlNameTerm acpi::AmlNodeBuffer::get(AmlNameId id) {
    ObjectHeader header = mHeaders[id.id];
    return *mObjects.get<AmlNameTerm>(header.offset);
}

using AmlNameFormat = km::Format<acpi::AmlName>;
using AmlConstFormat = km::Format<acpi::AmlData>;
using AmlTermTypeFormat = km::Format<acpi::AmlTermType>;

void AmlNameFormat::format(km::IOutStream& out, const acpi::AmlName& value) {
    if (value.segments.isEmpty()) {
        out.write("NullName");
        return;
    }

    if (value.prefix == SIZE_MAX) {
        out.write('\\');
    } else {
        for (size_t i = 0; i < value.prefix; i++) {
            out.write('^');
        }
    }

    KmDebugMessage("LENGTH: ", value.segments.count(), "\n");

    for (size_t i = 0; i < value.segments.count(); i++) {
        if (i != 0) out.write('.');
        out.write(value.segments[i]);
    }
}

void AmlConstFormat::format(km::IOutStream& out, const acpi::AmlData& value) {
    switch (value.type) {
    case acpi::AmlData::Type::eByte:
        out.format(km::Hex(std::get<uint64_t>(value.data)).pad(2));
        break;
    case acpi::AmlData::Type::eWord:
        out.format(km::Hex(std::get<uint64_t>(value.data)).pad(4));
        break;
    case acpi::AmlData::Type::eDword:
        out.format(km::Hex(std::get<uint64_t>(value.data)).pad(8));
        break;
    case acpi::AmlData::Type::eQword:
        out.format(km::Hex(std::get<uint64_t>(value.data)).pad(16));
        break;
    case acpi::AmlData::Type::eString:
        out.format("\"", std::get<stdx::StringView>(value.data), "\"");
        break;
    case acpi::AmlData::Type::ePackage:
        out.write("package");
        break;
    case acpi::AmlData::Type::eBuffer:
        out.write("buffer");
        break;
    default:
        out.write("unknown");
    }
}

void AmlTermTypeFormat::format(km::IOutStream& out, acpi::AmlTermType value) {
    switch (value) {
    case acpi::AmlTermType::eName:
        out.write("Name");
        break;
    case acpi::AmlTermType::eAlias:
        out.write("Alias");
        break;
    case acpi::AmlTermType::eScope:
        out.write("Scope");
        break;
    case acpi::AmlTermType::eMethod:
        out.write("Method");
        break;
    case acpi::AmlTermType::eIfElse:
        out.write("IfElse");
        break;
    case acpi::AmlTermType::eStore:
        out.write("Store");
        break;
    case acpi::AmlTermType::eOpRegion:
        out.write("OpRegion");
        break;
    case acpi::AmlTermType::eDevice:
        out.write("Device");
        break;
    case acpi::AmlTermType::eProcessor:
        out.write("Processor");
        break;
    case acpi::AmlTermType::eData:
        out.write("Data");
        break;
    case acpi::AmlTermType::ePackage:
        out.write("Package");
        break;
    default:
        out.write(std::to_underlying(value));
    }
}
