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

static uint8_t GetNameChar(acpi::AmlParser& parser, acpi::AmlNodeBuffer& code) {
    uint8_t next = parser.read();
    if (!IsNameChar(next)) {
        ReportError(parser, code, "Invalid name character ", km::Hex(next).pad(2, '0'));
    }

    return next;
}

static void NameSegBody(acpi::AmlParser& parser, acpi::AmlNodeBuffer& code, acpi::AmlName& dst, char lead) {
    if (lead != '\\') {
        if (!IsLeadNameChar(lead)) {
            ReportError(parser, code, "Invalid lead name character ", km::Hex((uint8_t)lead).pad(2, '0'));
        }
        char a = GetNameChar(parser, code);
        char b = GetNameChar(parser, code);
        char c = GetNameChar(parser, code);

        dst.segments.add({ lead, a, b, c });
    } else {
        char a = GetNameChar(parser, code);
        char b = GetNameChar(parser, code);
        char c = GetNameChar(parser, code);
        char d = GetNameChar(parser, code);

        (void)d; // discard the last character

        dst.segments.add({ lead, a, b, c });
    }
}

static void NameSeg(acpi::AmlParser& parser, acpi::AmlNodeBuffer& code, acpi::AmlName& dst) {
    char lead = parser.read();
    NameSegBody(parser, code, dst, lead);
}

enum {
    kPrefixChar = 0x5e,
    kRootChar = 0x5c,

    kDualNamePrefix = 0x2e,
    kMultiNamePrefix = 0x2f,
    kNullName = 0x00,
};

static void NamePath(acpi::AmlParser& parser, acpi::AmlNodeBuffer& code, acpi::AmlName& dst) {
    uint8_t it = parser.read();
    switch (it) {
    case kMultiNamePrefix: {
        uint8_t count = parser.read();
        for (uint8_t i = 0; i < count; i++) {
            NameSeg(parser, code, dst);
        }
        break;
    }
    case kDualNamePrefix:
        NameSeg(parser, code, dst);
        NameSeg(parser, code, dst);
        break;

    case kNullName:
        break;

    default:
        NameSegBody(parser, code, dst, it);
        break;
    }
}

static void PrefixPath(acpi::AmlParser& parser, acpi::AmlName& name) {
    while (parser.consume(kPrefixChar)) {
        name.prefix += 1;
    }
}

acpi::AmlName acpi::detail::NameString(acpi::AmlParser& parser, AmlNodeBuffer& code) {
    acpi::AmlName name = { 0 };

    uint8_t letter = parser.peek();
    switch (letter) {
    case kRootChar:
        parser.advance();
        name.useroot = true;
        NamePath(parser, code, name);
        break;

    default:
        PrefixPath(parser, name);
        NamePath(parser, code, name);
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

acpi::AmlBuffer acpi::detail::BufferData(acpi::AmlParser& parser, acpi::AmlNodeBuffer& code) {
    size_t front = parser.offset();

    uint32_t length = PkgLength(parser);

    acpi::AmlAnyId size = TermArg(parser, code);
    size_t back = parser.offset();

    size_t space = length - (back - front);

    stdx::Vector<uint8_t> data(code.allocator(), length);
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

    stdx::Vector<acpi::AmlAnyId> terms(code.allocator());

    if (length != 0) {
        acpi::AmlParser sub = parser.subspan(length - (back - front));

        while (!sub.done()) {
            terms.add(DataRefObject(sub, code));
        }
    }

    return code.add(acpi::AmlPackageTerm { count, terms });
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

    kArg0Op = 0x68,
    kArg1Op = 0x69,
    kArg2Op = 0x6A,
    kArg3Op = 0x6B,
    kArg4Op = 0x6C,
    kArg5Op = 0x6D,
    kArg6Op = 0x6E,

    kLocal0Op = 0x60,
    kLocal1Op = 0x61,
    kLocal2Op = 0x62,
    kLocal3Op = 0x63,
    kLocal4Op = 0x64,
    kLocal5Op = 0x65,
    kLocal6Op = 0x66,
    kLocal7Op = 0x67,

    kAliasOp = 0x06,
    kNameOp = 0x08,
    kScopeOp = 0x10,
    kCondRefOp = 0x12,
    kMethodOp = 0x14,
    kExternalOp = 0x15,
    kStoreOp = 0x70,
    kIfOp = 0xA0,
    kElseOp = 0xA1,
    kWhileOp = 0xA2,

    kReturnOp = 0xA4,

    kDebugOp = 0x31,

    kBreakOp = 0xA5,
    kBreakPointOp = 0xCC,
    kContinueOp = 0x9F,
    kFatalOp = 0x32,
    kToIntegerOp = 0x99,
    kToStringOp = 0x9C,
    kDivideOp = 0x78,
    kMultiplyOp = 0x77,

    kLAndOp = 0x90,
    kLOrOp = 0x91,
    kLNotOp = 0x92,
    kLEqualOp = 0x93,
    kLGreaterOp = 0x94,
    kLLessOp = 0x95,

    kSizeOfOp = 0x87,
    kAddOp = 0x72,
    kAndOp = 0x7B,
    kOrOp = 0x7D,
    kIncrementOp = 0x75,
    kDecrementOp = 0x76,

    kCreateByteFieldOp = 0x8C,
    kCreateWordFieldOp = 0x8B,
    kCreateDwordFieldOp = 0x8A,
    kCreateQwordFieldOp = 0x8F,

    kExtOpPrefix = 0x5B,

    kRegionOp = 0x80,
    kFieldOp = 0x81,
    kDeviceOp = 0x82,
    kProcessorOp = 0x83,
    kDerefOfOp = 0x83,
    kIndexFieldOp = 0x86,
    kPowerResOp = 0x84,
    kThermalZoneOp = 0x85,
    kMutexOp = 0x01,
    kToHexStringOp = 0x98,
    kToBufferOp = 0x96,
    kSubtractOp = 0x74,
    kIndexOp = 0x88,
    kAcquireOp = 0x23,
    kReleaseOp = 0x27,
    kNotifyOp = 0x86,
    kStallOp = 0x21,
    kSleepOp = 0x22,

    kShiftLeftOp = 0x79,
    kShiftRightOp = 0x7A,

    kFindSetLeftBitOp = 0x81,
    kFindSetRightBitOp = 0x82,
};

struct AmlDataPair {
    uint8_t op;
    acpi::AmlData data;
};

static std::optional<acpi::AmlLocalObject> LocalObject(acpi::AmlParser& parser) {
    struct LocalPair {
        uint8_t op;
        acpi::AmlLocalObject data;
    };

    static constexpr LocalPair kLocalPairs[] = {
        { kLocal0Op, acpi::AmlLocalObject::eLocal0 },
        { kLocal1Op, acpi::AmlLocalObject::eLocal1 },
        { kLocal2Op, acpi::AmlLocalObject::eLocal2 },
        { kLocal3Op, acpi::AmlLocalObject::eLocal3 },
        { kLocal4Op, acpi::AmlLocalObject::eLocal4 },
        { kLocal5Op, acpi::AmlLocalObject::eLocal5 },
        { kLocal6Op, acpi::AmlLocalObject::eLocal6 },
        { kLocal7Op, acpi::AmlLocalObject::eLocal7 },
    };

    for (const auto& pair : kLocalPairs) {
        if (parser.consumeSeq(pair.op)) {
            return pair.data;
        }
    }

    return std::nullopt;
}

static std::optional<acpi::AmlArgObject> ArgObject(acpi::AmlParser& parser) {
    struct ArgPair {
        uint8_t op;
        acpi::AmlArgObject data;
    };

    static constexpr ArgPair kArgPairs[] = {
        { kArg0Op, acpi::AmlArgObject::eArg0 },
        { kArg1Op, acpi::AmlArgObject::eArg1 },
        { kArg2Op, acpi::AmlArgObject::eArg2 },
        { kArg3Op, acpi::AmlArgObject::eArg3 },
        { kArg4Op, acpi::AmlArgObject::eArg4 },
        { kArg5Op, acpi::AmlArgObject::eArg5 },
        { kArg6Op, acpi::AmlArgObject::eArg6 },
    };

    for (const auto& pair : kArgPairs) {
        if (parser.consumeSeq(pair.op)) {
            return pair.data;
        }
    }

    return std::nullopt;
}

static std::optional<acpi::AmlAnyId> StatementOpcode(acpi::AmlParser& parser, acpi::AmlNodeBuffer& code) {
    if (parser.consumeSeq(kBreakOp))
        return code.add(DefBreak(parser, code));

    if (parser.consumeSeq(kBreakPointOp))
        return code.add(DefBreakPoint(parser, code));

    if (parser.consumeSeq(kContinueOp))
        return code.add(DefContinue(parser, code));

    if (parser.consumeSeq(kToStringOp))
        return code.add(DefToString(parser, code));

    if (parser.consumeSeq(kExtOpPrefix, kFatalOp))
        return code.add(DefFatal(parser, code));

    if (parser.consumeSeq(kIfOp))
        return code.add(DefIfElse(parser, code));

    if (parser.consumeSeq(kElseOp))
        return code.add(DefElse(parser, code));

    if (parser.consumeSeq(kReturnOp))
        return code.add(DefReturn(parser, code));

    if (parser.consumeSeq(kWhileOp))
        return code.add(DefWhile(parser, code));

    if (parser.consumeSeq(kExtOpPrefix, kReleaseOp))
        return code.add(DefRelease(parser, code));

    if (parser.consumeSeq(kNotifyOp))
        return code.add(DefNotify(parser, code));

    if (parser.consumeSeq(kExtOpPrefix, kStallOp))
        return code.add(DefStall(parser, code));

    if (parser.consumeSeq(kExtOpPrefix, kSleepOp))
        return code.add(DefSleep(parser, code));

    return std::nullopt;
}

static std::optional<acpi::AmlAnyId> ExpressionOpcode(acpi::AmlParser& parser, acpi::AmlNodeBuffer& code) {
    auto addTerm = [&](acpi::AmlData::Type type, auto data) {
        return code.add(acpi::AmlDataTerm { { type, { data } } });
    };

    if (parser.consumeSeq(kZeroOp))
        return addTerm(acpi::AmlData::Type::eByte, uint64_t(0));

    if (parser.consumeSeq(kOneOp))
        return addTerm(acpi::AmlData::Type::eByte, uint64_t(1));

    if (parser.consumeSeq(kMultiplyOp))
        return code.add(DefMultiply(parser, code));

    if (parser.consumeSeq(kLNotOp))
        return code.add(DefLNot(parser, code));

    if (parser.consumeSeq(kLOrOp))
        return code.add(DefLOr(parser, code));

    if (parser.consumeSeq(kLAndOp))
        return code.add(DefLAnd(parser, code));

    if (parser.consumeSeq(kLEqualOp))
        return code.add(DefLEqual(parser, code));

    if (parser.consumeSeq(kDivideOp))
        return code.add(DefDivide(parser, code));

    if (parser.consumeSeq(kLLessOp))
        return code.add(DefLLess(parser, code));

    if (parser.consumeSeq(kLGreaterOp))
        return code.add(DefLGreater(parser, code));

    if (parser.consumeSeq(kSizeOfOp))
        return code.add(DefSizeOf(parser, code));

    if (parser.consumeSeq(kExtOpPrefix, kCondRefOp))
        return code.add(ConfRefOf(parser, code));

    if (parser.consumeSeq(kAddOp))
        return code.add(DefAdd(parser, code));

    if (parser.consumeSeq(kToHexStringOp))
        return code.add(DefToHexString(parser, code));

    if (parser.consumeSeq(kToBufferOp))
        return code.add(DefToBuffer(parser, code));

    if (parser.consumeSeq(kIncrementOp))
        return code.add(DefIncrement(parser, code));

    if (parser.consumeSeq(kDecrementOp))
        return code.add(DefDecrement(parser, code));

    if (parser.consumeSeq(kDerefOfOp))
        return code.add(DefDerefOf(parser, code));

    if (parser.consumeSeq(kSubtractOp))
        return code.add(DefSubtract(parser, code));

    if (parser.consumeSeq(kIndexOp))
        return code.add(DefIndex(parser, code));

    if (parser.consumeSeq(kAndOp))
        return code.add(DefAnd(parser, code));

    if (parser.consumeSeq(kOrOp))
        return code.add(DefOr(parser, code));

    if (parser.consumeSeq(kExtOpPrefix, kAcquireOp))
        return code.add(DefAcquire(parser, code));

    if (parser.consumeSeq(kShiftLeftOp))
        return code.add(DefShiftLeft(parser, code));

    if (parser.consumeSeq(kShiftRightOp))
        return code.add(DefShiftRight(parser, code));

    if (parser.consumeSeq(kFindSetLeftBitOp))
        return code.add(DefFindSetLeftBit(parser, code));

    if (parser.consumeSeq(kFindSetRightBitOp))
        return code.add(DefFindSetRightBit(parser, code));

    if (parser.consumeSeq(kToIntegerOp))
        return code.add(DefToInteger(parser, code));

    return std::nullopt;
}

static std::optional<acpi::AmlAnyId> ComputationalData(acpi::AmlParser& parser, acpi::AmlNodeBuffer& code) {
    auto addTerm = [&](acpi::AmlData::Type type, auto data) {
        return code.add(acpi::AmlDataTerm { { type, { data } } });
    };

    if (parser.consumeSeq(kBytePrefix))
        return addTerm(acpi::AmlData::Type::eByte, ByteData(parser));

    if (parser.consumeSeq(kWordPrefix))
        return addTerm(acpi::AmlData::Type::eWord, WordData(parser));

    if (parser.consumeSeq(kDwordPrefix))
        return addTerm(acpi::AmlData::Type::eDword, DwordData(parser));

    if (parser.consumeSeq(kQwordPrefix))
        return addTerm(acpi::AmlData::Type::eQword, QwordData(parser));

    if (parser.consumeSeq(kStringPrefix))
        return addTerm(acpi::AmlData::Type::eString, StringData(parser));

    if (parser.consumeSeq(kBufferPrefix))
        return addTerm(acpi::AmlData::Type::eBuffer, BufferData(parser, code));

    if (parser.consumeSeq(kPackageOp))
        return addTerm(acpi::AmlData::Type::ePackage, DefPackage(parser, code));

    if (parser.consumeSeq(kOnesOp))
        return code.add(acpi::AmlDataTerm { code.ones() });

    return std::nullopt;
}

acpi::AmlAnyId acpi::detail::DataRefObject(acpi::AmlParser& parser, acpi::AmlNodeBuffer& code) {
    auto addTerm = [&](acpi::AmlData::Type type, auto data) {
        return code.add(AmlDataTerm { { type, { data } } });
    };

    if (parser.consumeSeq(kZeroOp))
        return addTerm(acpi::AmlData::Type::eByte, uint64_t(0));

    if (parser.consumeSeq(kOneOp))
        return addTerm(acpi::AmlData::Type::eByte, uint64_t(1));

    if (auto cd = ComputationalData(parser, code)) {
        return *cd;
    }

    switch (parser.peek()) {
    case 'A'...'Z': case '_': case kRootChar:
        return code.add(DefMethodInvoke(parser, code));

    default:
        break;
    }

    if (auto arg = ArgObject(parser)) {
        return addTerm(acpi::AmlData::Type::eArg, *arg);
    }

    if (auto local = LocalObject(parser)) {
        return addTerm(acpi::AmlData::Type::eLocal, *local);
    }

    ReportError(parser, code, "Invalid data object ", km::Hex(parser.peek()).pad(2, '0'));

    return AmlAnyId::kInvalid;
}

acpi::AmlAnyId acpi::detail::TermArg(AmlParser& parser, acpi::AmlNodeBuffer& code) {
    auto addTerm = [&](acpi::AmlData::Type type, auto data) {
        return code.add(AmlDataTerm { { type, { data } } });
    };

    if (auto arg = ArgObject(parser)) {
        return addTerm(acpi::AmlData::Type::eArg, *arg);
    }

    if (auto local = LocalObject(parser)) {
        return addTerm(acpi::AmlData::Type::eLocal, *local);
    }

    if (auto expr = ExpressionOpcode(parser, code)) {
        return *expr;
    }

    return DataRefObject(parser, code);
}

static acpi::AmlMethodFlags MethodFlags(acpi::AmlParser& parser) {
    uint8_t flags = parser.read();
    return { flags };
}

static acpi::AmlFieldFlags FieldFlags(acpi::AmlParser& parser) {
    uint8_t flags = parser.read();
    return { flags };
}

stdx::Vector<acpi::AmlAnyId> acpi::detail::TermList(AmlParser& parser, AmlNodeBuffer& code) {
    stdx::Vector<acpi::AmlAnyId> terms(code.allocator());

    while (!parser.done()) {
        acpi::AmlAnyId term = Term(parser, code);
        if (code.error() || term == acpi::AmlAnyId::kInvalid)
            break;
        terms.add(term);
    }

    if (terms.count() == 0) {
        return terms;
    }

    // if theres an else term following a branch term, merge them
    for (size_t i = 0; i < terms.count() - 1; i++) {
        acpi::AmlBranchTerm *branch = code.get<acpi::AmlBranchTerm>(terms[i]);
        acpi::AmlElseTerm *otherwise = code.get<acpi::AmlElseTerm>(terms[i + 1]);

        if (branch && otherwise) {
            branch->otherwise = terms[i + 1];

            terms.remove(i + 1);
        }
    }

    return terms;
}

acpi::AmlNameTerm acpi::detail::NameTerm(AmlParser& parser, AmlNodeBuffer& code) {
    acpi::AmlName name = NameString(parser, code);
    acpi::AmlAnyId data = DataRefObject(parser, code);

    return acpi::AmlNameTerm { name, data };
}

static acpi::AmlSimpleName SimpleName(acpi::AmlParser& parser, acpi::AmlNodeBuffer& code) {
    if (auto local = LocalObject(parser)) {
        return acpi::AmlSimpleName { *local };
    }

    if (auto arg = ArgObject(parser)) {
        return acpi::AmlSimpleName { *arg };
    }

    return acpi::AmlSimpleName { NameString(parser, code) };
}

static std::optional<acpi::AmlAnyId> ReferenceType(acpi::AmlParser& parser, acpi::AmlNodeBuffer& code) {
    if (parser.consumeSeq(kDerefOfOp))
        return code.add(DefDerefOf(parser, code));

    if (parser.consumeSeq(kIndexOp))
        return code.add(DefIndex(parser, code));

    return std::nullopt;
}

acpi::AmlSuperName acpi::detail::SuperName(AmlParser& parser, AmlNodeBuffer& code) {
    if (parser.consumeSeq(kExtOpPrefix, kDebugOp))
        return acpi::AmlDebugObject { };

    if (auto ref = ReferenceType(parser, code)) {
        return *ref;
    }

    return std::visit([&](auto&& it) -> acpi::AmlSuperName { return it; }, SimpleName(parser, code));
}

acpi::AmlTarget acpi::detail::Target(AmlParser& parser, AmlNodeBuffer& code) {
    if (parser.consume('\0')) {
        return AmlNullName{};
    }

    return std::visit([&](auto&& it) -> acpi::AmlTarget { return it; }, SuperName(parser, code));
}

acpi::AmlScopeTerm acpi::detail::ScopeTerm(AmlParser& parser, AmlNodeBuffer& code) {
    uint32_t front = parser.offset();

    uint32_t length = PkgLength(parser);
    acpi::AmlName name = NameString(parser, code);

    uint32_t back = parser.offset();

    acpi::AmlParser sub = parser.subspan(length - (back - front));
    stdx::Vector<acpi::AmlAnyId> terms = TermList(sub, code);

    return acpi::AmlScopeTerm { name, terms };
}

acpi::AmlMethodTerm acpi::detail::MethodTerm(AmlParser& parser, AmlNodeBuffer& code) {
    uint32_t front = parser.offset();

    uint32_t length = PkgLength(parser);
    acpi::AmlName name = NameString(parser, code);
    AmlMethodFlags flags = MethodFlags(parser);

    uint32_t back = parser.offset();

    acpi::AmlParser sub = parser.subspan(length - (back - front));

    stdx::Vector<acpi::AmlAnyId> terms = TermList(sub, code);

    return acpi::AmlMethodTerm { name, flags, terms };
}

acpi::AmlAliasTerm acpi::detail::AliasTerm(AmlParser& parser, AmlNodeBuffer& code) {
    acpi::AmlName name = NameString(parser, code);
    acpi::AmlName alias = NameString(parser, code);

    return acpi::AmlAliasTerm { name, alias };
}

acpi::AmlDeviceTerm acpi::detail::DeviceTerm(AmlParser& parser, AmlNodeBuffer& code) {
    uint32_t front = parser.offset();
    uint32_t length = PkgLength(parser);
    acpi::AmlName name = NameString(parser, code);
    uint32_t back = parser.offset();

    acpi::AmlParser sub = parser.subspan(length - (back - front));

    stdx::Vector<AmlAnyId> terms = TermList(sub, code);

    return acpi::AmlDeviceTerm { name, terms };
}

acpi::AmlProcessorTerm acpi::detail::ProcessorTerm(AmlParser& parser, AmlNodeBuffer& code) {
    uint32_t front = parser.offset();
    uint32_t length = PkgLength(parser);
    acpi::AmlName name = NameString(parser, code);
    uint8_t id = ByteData(parser);
    uint32_t pblkAddr = DwordData(parser);
    uint8_t pblkLen = ByteData(parser);

    uint32_t back = parser.offset();

    acpi::AmlParser sub = parser.subspan(length - (back - front));

    stdx::Vector<AmlAnyId> terms = TermList(sub, code);

    return acpi::AmlProcessorTerm { name, id, pblkAddr, pblkLen, terms };
}

acpi::AmlBranchTerm acpi::detail::DefIfElse(AmlParser& parser, AmlNodeBuffer& code) {
    uint32_t front = parser.offset();

    uint32_t length = PkgLength(parser);

    AmlAnyId predicate = TermArg(parser, code);

    uint32_t back = parser.offset();

    acpi::AmlParser sub = parser.subspan(length - (back - front));

    stdx::Vector<acpi::AmlAnyId> terms = TermList(sub, code);

    return acpi::AmlBranchTerm { predicate, terms };
}

acpi::AmlElseTerm acpi::detail::DefElse(AmlParser& parser, AmlNodeBuffer& code) {
    uint32_t front = parser.offset();

    uint32_t length = PkgLength(parser);

    uint32_t back = parser.offset();

    acpi::AmlParser sub = parser.subspan(length - (back - front));

    stdx::Vector<acpi::AmlAnyId> terms = TermList(sub, code);

    return acpi::AmlElseTerm { terms };
}

acpi::AmlReturnTerm acpi::detail::DefReturn(AmlParser& parser, AmlNodeBuffer& code) {
    acpi::AmlAnyId value = TermArg(parser, code);

    return acpi::AmlReturnTerm { value };
}

acpi::AmlWhileTerm acpi::detail::DefWhile(AmlParser& parser, AmlNodeBuffer& code) {
    uint32_t front = parser.offset();

    uint32_t length = PkgLength(parser);
    AmlAnyId predicate = TermArg(parser, code);

    uint32_t back = parser.offset();

    acpi::AmlParser sub = parser.subspan(length - (back - front));

    stdx::Vector<AmlAnyId> terms = TermList(sub, code);

    return acpi::AmlWhileTerm { predicate, terms };
}

acpi::AmlMethodInvokeTerm acpi::detail::DefMethodInvoke(AmlParser& parser, AmlNodeBuffer& code) {
    acpi::AmlName name = NameString(parser, code);

    stdx::Vector<AmlAnyId> args(code.allocator());

    return acpi::AmlMethodInvokeTerm { name, args };
}

acpi::AmlCondRefOfTerm acpi::detail::ConfRefOf(AmlParser& parser, AmlNodeBuffer& code) {
    acpi::AmlSuperName name = SuperName(parser, code);
    acpi::AmlTarget target = Target(parser, code);

    return acpi::AmlCondRefOfTerm { name, target };
}

acpi::AmlOpRegionTerm acpi::detail::DefOpRegion(AmlParser& parser, AmlNodeBuffer& code) {
    acpi::AmlName name = NameString(parser, code);
    acpi::AddressSpaceId region = acpi::AddressSpaceId(ByteData(parser));
    acpi::AmlAnyId offset = TermArg(parser, code);
    acpi::AmlAnyId size = TermArg(parser, code);

    return acpi::AmlOpRegionTerm { name, region, offset, size };
}

acpi::AmlMutexTerm acpi::detail::DefMutex(AmlParser& parser, AmlNodeBuffer& code) {
    acpi::AmlName name = NameString(parser, code);
    uint8_t flags = ByteData(parser);

    return acpi::AmlMutexTerm { name, flags };
}

acpi::AmlFieldTerm acpi::detail::DefField(AmlParser& parser, AmlNodeBuffer& code) {
    uint32_t front = parser.offset();

    uint32_t length = PkgLength(parser);
    acpi::AmlName name = NameString(parser, code);
    AmlFieldFlags flags = FieldFlags(parser);

    uint32_t back = parser.offset();

    parser.skip(length - (back - front));

    return AmlFieldTerm { name, flags };
}

acpi::AmlThermalZoneTerm acpi::detail::DefThermalZone(AmlParser& parser, AmlNodeBuffer& code) {
    uint32_t front = parser.offset();

    uint32_t length = PkgLength(parser);
    acpi::AmlName name = NameString(parser, code);

    uint32_t back = parser.offset();

    AmlParser sub = parser.subspan(length - (back - front));

    stdx::Vector<AmlAnyId> terms = TermList(sub, code);

    return acpi::AmlThermalZoneTerm { name, terms };
}

acpi::AmlIndexFieldTerm acpi::detail::DefIndexField(AmlParser& parser, AmlNodeBuffer& code) {
    uint32_t front = parser.offset();

    uint32_t length = PkgLength(parser);
    acpi::AmlName name = NameString(parser, code);
    acpi::AmlName field = NameString(parser, code);
    AmlFieldFlags flags = FieldFlags(parser);

    uint32_t back = parser.offset();

    parser.skip(length - (back - front));

    return AmlIndexFieldTerm{ name, field, flags };
}

acpi::AmlPowerResTerm acpi::detail::DefPowerRes(AmlParser& parser, AmlNodeBuffer& code) {
    uint32_t front = parser.offset();

    uint32_t length = PkgLength(parser);
    acpi::AmlName name = NameString(parser, code);
    uint8_t level = ByteData(parser);
    uint16_t order = WordData(parser);

    uint32_t back = parser.offset();

    AmlParser sub = parser.subspan(length - (back - front));

    stdx::Vector<AmlAnyId> terms = TermList(sub, code);

    return acpi::AmlPowerResTerm { name, level, order, terms };
}

acpi::AmlExternalTerm acpi::detail::DefExternal(AmlParser& parser, AmlNodeBuffer& code) {
    acpi::AmlName name = NameString(parser, code);
    uint8_t type = ByteData(parser);
    uint8_t args = ByteData(parser);

    return acpi::AmlExternalTerm { name, type, args };
}

acpi::AmlStoreTerm acpi::detail::DefStore(AmlParser& parser, AmlNodeBuffer& code) {
    acpi::AmlAnyId source = TermArg(parser, code);
    acpi::AmlSuperName target = SuperName(parser, code);

    return acpi::AmlStoreTerm { source, target };
}

acpi::AmlCreateByteFieldTerm acpi::detail::DefCreateByteField(AmlParser& parser, AmlNodeBuffer& code) {
    AmlAnyId source = TermArg(parser, code);
    AmlAnyId index = TermArg(parser, code);
    acpi::AmlName name = NameString(parser, code);

    return acpi::AmlCreateByteFieldTerm { source, index, name };
}

acpi::AmlCreateWordFieldTerm acpi::detail::DefCreateWordField(AmlParser& parser, AmlNodeBuffer& code) {
    AmlAnyId source = TermArg(parser, code);
    AmlAnyId index = TermArg(parser, code);
    acpi::AmlName name = NameString(parser, code);

    return acpi::AmlCreateWordFieldTerm { source, index, name };
}

acpi::AmlCreateDwordFieldTerm acpi::detail::DefCreateDwordField(AmlParser& parser, AmlNodeBuffer& code) {
    AmlAnyId source = TermArg(parser, code);
    AmlAnyId index = TermArg(parser, code);
    acpi::AmlName name = NameString(parser, code);

    return acpi::AmlCreateDwordFieldTerm { source, index, name };
}

acpi::AmlCreateQwordFieldTerm acpi::detail::DefCreateQwordField(AmlParser& parser, AmlNodeBuffer& code) {
    AmlAnyId source = TermArg(parser, code);
    AmlAnyId index = TermArg(parser, code);
    acpi::AmlName name = NameString(parser, code);

    return acpi::AmlCreateQwordFieldTerm { source, index, name };
}

acpi::AmlUnaryTerm acpi::detail::DefLNot(AmlParser& parser, AmlNodeBuffer& code) {
    AmlAnyId term = TermArg(parser, code);

    return acpi::AmlUnaryTerm { acpi::AmlUnaryTerm::eNot, term };
}

acpi::AmlBinaryTerm acpi::detail::DefLEqual(AmlParser& parser, AmlNodeBuffer& code) {
    AmlAnyId left = TermArg(parser, code);
    AmlAnyId right = TermArg(parser, code);

    return acpi::AmlBinaryTerm { acpi::AmlBinaryTerm::eEqual, left, right };
}

acpi::AmlBinaryTerm acpi::detail::DefLGreater(AmlParser& parser, AmlNodeBuffer& code) {
    AmlAnyId left = TermArg(parser, code);
    AmlAnyId right = TermArg(parser, code);

    return acpi::AmlBinaryTerm { acpi::AmlBinaryTerm::eGreater, left, right };
}

acpi::AmlBinaryTerm acpi::detail::DefLOr(AmlParser& parser, AmlNodeBuffer& code) {
    AmlAnyId left = TermArg(parser, code);
    AmlAnyId right = TermArg(parser, code);

    return acpi::AmlBinaryTerm { acpi::AmlBinaryTerm::eOr, left, right };
}

acpi::AmlBinaryTerm acpi::detail::DefLAnd(AmlParser& parser, AmlNodeBuffer& code) {
    AmlAnyId left = TermArg(parser, code);
    AmlAnyId right = TermArg(parser, code);

    return acpi::AmlBinaryTerm { acpi::AmlBinaryTerm::eAnd, left, right };
}

acpi::AmlBinaryTacTerm acpi::detail::DefAdd(AmlParser& parser, AmlNodeBuffer& code) {
    AmlAnyId left = TermArg(parser, code);
    AmlAnyId right = TermArg(parser, code);
    AmlTarget target = Target(parser, code);

    return acpi::AmlBinaryTacTerm { acpi::AmlBinaryTacTerm::eAdd, left, right, target };
}

acpi::AmlBinaryTacTerm acpi::detail::DefMultiply(AmlParser& parser, AmlNodeBuffer& code) {
    AmlAnyId left = TermArg(parser, code);
    AmlAnyId right = TermArg(parser, code);
    AmlTarget target = Target(parser, code);

    return acpi::AmlBinaryTacTerm { acpi::AmlBinaryTacTerm::eMul, left, right, target };
}

acpi::AmlBinaryTerm acpi::detail::DefLLess(AmlParser& parser, AmlNodeBuffer& code) {
    AmlAnyId left = TermArg(parser, code);
    AmlAnyId right = TermArg(parser, code);

    return acpi::AmlBinaryTerm { acpi::AmlBinaryTerm::eLess, left, right };
}

acpi::AmlSizeOfTerm acpi::detail::DefSizeOf(AmlParser& parser, AmlNodeBuffer& code) {
    AmlSuperName name = SuperName(parser, code);

    return acpi::AmlSizeOfTerm { name };
}

acpi::AmlUnaryTacTerm acpi::detail::DefToHexString(AmlParser& parser, AmlNodeBuffer& code) {
    AmlAnyId term = TermArg(parser, code);
    AmlTarget target = Target(parser, code);

    return acpi::AmlUnaryTacTerm { acpi::AmlUnaryTacTerm::eToHexString, term, target };
}

acpi::AmlUnaryTacTerm acpi::detail::DefToBuffer(AmlParser& parser, AmlNodeBuffer& code) {
    AmlAnyId term = TermArg(parser, code);
    AmlTarget target = Target(parser, code);

    return acpi::AmlUnaryTacTerm { acpi::AmlUnaryTacTerm::eToBuffer, term, target };
}

acpi::AmlUnaryTacTerm acpi::detail::DefToInteger(AmlParser& parser, AmlNodeBuffer& code) {
    AmlAnyId term = TermArg(parser, code);
    AmlTarget target = Target(parser, code);

    return acpi::AmlUnaryTacTerm { acpi::AmlUnaryTacTerm::eToInteger, term, target };
}

acpi::AmlToStringTerm acpi::detail::DefToString(AmlParser& parser, AmlNodeBuffer& code) {
    AmlAnyId term = TermArg(parser, code);
    AmlAnyId length = TermArg(parser, code);
    AmlTarget target = Target(parser, code);

    return acpi::AmlToStringTerm { term, length, target };
}

acpi::AmlExpression acpi::detail::DefDecrement(AmlParser& parser, AmlNodeBuffer& code) {
    AmlSuperName name = SuperName(parser, code);

    return acpi::AmlExpression { acpi::AmlExpression::eDecrement, name };
}

acpi::AmlExpression acpi::detail::DefIncrement(AmlParser& parser, AmlNodeBuffer& code) {
    AmlSuperName name = SuperName(parser, code);

    return acpi::AmlExpression { acpi::AmlExpression::eIncrement, name };
}

acpi::AmlUnaryTerm acpi::detail::DefDerefOf(AmlParser& parser, AmlNodeBuffer& code) {
    AmlAnyId name = TermArg(parser, code);

    return acpi::AmlUnaryTerm { acpi::AmlUnaryTerm::eDerefOf, name };
}

acpi::AmlBinaryTacTerm acpi::detail::DefSubtract(AmlParser& parser, AmlNodeBuffer& code) {
    AmlAnyId left = TermArg(parser, code);
    AmlAnyId right = TermArg(parser, code);
    AmlTarget target = Target(parser, code);

    return acpi::AmlBinaryTacTerm { acpi::AmlBinaryTacTerm::eSub, left, right, target };
}

acpi::AmlBinaryTacTerm acpi::detail::DefIndex(AmlParser& parser, AmlNodeBuffer& code) {
    AmlAnyId left = TermArg(parser, code);
    AmlAnyId right = TermArg(parser, code);
    AmlTarget target = Target(parser, code);

    return acpi::AmlBinaryTacTerm { acpi::AmlBinaryTacTerm::eIndex, left, right, target };
}

acpi::AmlBinaryTacTerm acpi::detail::DefAnd(AmlParser& parser, AmlNodeBuffer& code) {
    AmlAnyId left = TermArg(parser, code);
    AmlAnyId right = TermArg(parser, code);
    AmlTarget target = Target(parser, code);

    return acpi::AmlBinaryTacTerm { acpi::AmlBinaryTacTerm::eAnd, left, right, target };
}

acpi::AmlBinaryTacTerm acpi::detail::DefOr(AmlParser& parser, AmlNodeBuffer& code) {
    AmlAnyId left = TermArg(parser, code);
    AmlAnyId right = TermArg(parser, code);
    AmlTarget target = Target(parser, code);

    return acpi::AmlBinaryTacTerm { acpi::AmlBinaryTacTerm::eOr, left, right, target };
}

acpi::AmlBinaryTacTerm acpi::detail::DefShiftLeft(AmlParser& parser, AmlNodeBuffer& code) {
    AmlAnyId left = TermArg(parser, code);
    AmlAnyId right = TermArg(parser, code);
    AmlTarget target = Target(parser, code);

    return acpi::AmlBinaryTacTerm { acpi::AmlBinaryTacTerm::eShiftLeft, left, right, target };
}

acpi::AmlBinaryTacTerm acpi::detail::DefShiftRight(AmlParser& parser, AmlNodeBuffer& code) {
    AmlAnyId left = TermArg(parser, code);
    AmlAnyId right = TermArg(parser, code);
    AmlTarget target = Target(parser, code);

    return acpi::AmlBinaryTacTerm { acpi::AmlBinaryTacTerm::eShiftRight, left, right, target };
}

acpi::AmlAcquireTerm acpi::detail::DefAcquire(AmlParser& parser, AmlNodeBuffer& code) {
    AmlSuperName name = SuperName(parser, code);
    uint16_t timeout = WordData(parser);

    return acpi::AmlAcquireTerm { name, timeout };
}

acpi::AmlReleaseTerm acpi::detail::DefRelease(AmlParser& parser, AmlNodeBuffer& code) {
    AmlSuperName name = SuperName(parser, code);

    return acpi::AmlReleaseTerm { name };
}

acpi::AmlUnaryTacTerm acpi::detail::DefFindSetLeftBit(AmlParser& parser, AmlNodeBuffer& code) {
    AmlAnyId term = TermArg(parser, code);
    AmlTarget target = Target(parser, code);

    return acpi::AmlUnaryTacTerm { acpi::AmlUnaryTacTerm::eFindSetLeftBit, term, target };
}

acpi::AmlUnaryTacTerm acpi::detail::DefFindSetRightBit(AmlParser& parser, AmlNodeBuffer& code) {
    AmlAnyId term = TermArg(parser, code);
    AmlTarget target = Target(parser, code);

    return acpi::AmlUnaryTacTerm { acpi::AmlUnaryTacTerm::eFindSetRightBit, term, target };
}

acpi::AmlNotifyTerm acpi::detail::DefNotify(AmlParser& parser, AmlNodeBuffer& code) {
    AmlSuperName name = SuperName(parser, code);
    AmlAnyId value = TermArg(parser, code);

    return acpi::AmlNotifyTerm { name, value };
}

acpi::AmlStatementTerm acpi::detail::DefContinue(AmlParser&, AmlNodeBuffer&) {
    return acpi::AmlStatementTerm { acpi::AmlStatementTerm::eContinue };
}

acpi::AmlStatementTerm acpi::detail::DefBreak(AmlParser&, AmlNodeBuffer&) {
    return acpi::AmlStatementTerm { acpi::AmlStatementTerm::eBreak };
}

acpi::AmlStatementTerm acpi::detail::DefBreakPoint(AmlParser&, AmlNodeBuffer&) {
    return acpi::AmlStatementTerm { acpi::AmlStatementTerm::eBreakPoint };
}

acpi::AmlFatalTerm acpi::detail::DefFatal(AmlParser& parser, AmlNodeBuffer& code) {
    uint8_t type = ByteData(parser);
    uint32_t error = DwordData(parser);
    AmlAnyId arg = TermArg(parser, code);

    return acpi::AmlFatalTerm { type, error, arg };
}

acpi::AmlDivideTerm acpi::detail::DefDivide(AmlParser& parser, AmlNodeBuffer& code) {
    AmlAnyId dividend = TermArg(parser, code);
    AmlAnyId divisor = TermArg(parser, code);
    AmlTarget remainder = Target(parser, code);
    AmlTarget quotient = Target(parser, code);

    return acpi::AmlDivideTerm { dividend, divisor, remainder, quotient };
}

acpi::AmlWaitTerm acpi::detail::DefSleep(AmlParser& parser, AmlNodeBuffer& code) {
    AmlAnyId time = TermArg(parser, code);

    return acpi::AmlWaitTerm { acpi::AmlWaitTerm::eSleep, time };
}

acpi::AmlWaitTerm acpi::detail::DefStall(AmlParser& parser, AmlNodeBuffer& code) {
    AmlAnyId time = TermArg(parser, code);

    return acpi::AmlWaitTerm { acpi::AmlWaitTerm::eStall, time };
}

acpi::AmlAnyId acpi::detail::Term(acpi::AmlParser& parser, acpi::AmlNodeBuffer& code) {
    if (code.error()) {
        return acpi::AmlAnyId::kInvalid;
    }

    if (parser.consumeSeq(kNameOp))
        return code.add(NameTerm(parser, code));

    if (parser.consumeSeq(kAliasOp))
        return code.add(AliasTerm(parser, code));

    if (parser.consumeSeq(kScopeOp))
        return code.add(ScopeTerm(parser, code));

    if (parser.consumeSeq(kMethodOp))
        return code.add(MethodTerm(parser, code));

    if (parser.consumeSeq(kExternalOp))
        return code.add(DefExternal(parser, code));

    if (parser.consumeSeq(kIfOp))
        return code.add(DefIfElse(parser, code));

    if (parser.consumeSeq(kElseOp))
        return code.add(DefElse(parser, code));

    if (parser.consumeSeq(kStoreOp))
        return code.add(DefStore(parser, code));

    if (parser.consumeSeq(kCreateByteFieldOp))
        return code.add(DefCreateByteField(parser, code));

    if (parser.consumeSeq(kCreateWordFieldOp))
        return code.add(DefCreateWordField(parser, code));

    if (parser.consumeSeq(kCreateDwordFieldOp))
        return code.add(DefCreateDwordField(parser, code));

    if (parser.consumeSeq(kCreateQwordFieldOp))
        return code.add(DefCreateQwordField(parser, code));

    if (parser.consumeSeq(kExtOpPrefix, kFieldOp))
        return code.add(DefField(parser, code));

    if (parser.consumeSeq(kExtOpPrefix, kDeviceOp))
        return code.add(DeviceTerm(parser, code));

    if (parser.consumeSeq(kExtOpPrefix, kProcessorOp))
        return code.add(ProcessorTerm(parser, code));

    if (parser.consumeSeq(kExtOpPrefix, kIndexFieldOp))
        return code.add(DefIndexField(parser, code));

    if (parser.consumeSeq(kExtOpPrefix, kPowerResOp))
        return code.add(DefPowerRes(parser, code));

    if (parser.consumeSeq(kExtOpPrefix, kThermalZoneOp))
        return code.add(DefThermalZone(parser, code));

    if (parser.consumeSeq(kExtOpPrefix, kRegionOp))
        return code.add(DefOpRegion(parser, code));

    if (parser.consumeSeq(kExtOpPrefix, kMutexOp))
        return code.add(DefMutex(parser, code));

    if (auto cd = ComputationalData(parser, code)) {
        return *cd;
    }

    switch (parser.peek()) {
    case 'A'...'Z': case '_': case kRootChar:
        return code.add(DefMethodInvoke(parser, code));

    default:
        break;
    }

    if (auto stmt = StatementOpcode(parser, code)) {
        return *stmt;
    }

    if (auto expr = ExpressionOpcode(parser, code)) {
        return *expr;
    }

    auto addTerm = [&](acpi::AmlData::Type type, auto data) {
        return code.add(acpi::AmlDataTerm { { type, { data } } });
    };

    if (auto local = LocalObject(parser)) {
        return addTerm(acpi::AmlData::Type::eLocal, *local);
    }

    if (auto arg = ArgObject(parser)) {
        return addTerm(acpi::AmlData::Type::eArg, *arg);
    }

    code.setInvalid();
    if (parser.consume(kExtOpPrefix)) {
        ReportError(parser, code, "Unknown ext term: ", km::Hex(parser.peek()).pad(2, '0'));
    } else {
        ReportError(parser, code, "Unknown term: ", km::Hex(parser.peek()).pad(2, '0'));
    }
    return acpi::AmlAnyId::kInvalid;
}

acpi::AmlCode acpi::WalkAml(const RsdtHeader *dsdt, mem::IAllocator *arena) {
    AmlCode code = { *dsdt, arena };

    AmlParser parser(dsdt);
    AmlNodeBuffer& nodes = code.nodes();

    stdx::Vector<AmlAnyId> terms = TermList(parser, nodes);

    code.root()->terms = terms;

    return code;
}

using AmlNameFormat = km::Format<acpi::AmlName>;
using AmlConstFormat = km::Format<acpi::AmlData>;
using AmlTermTypeFormat = km::Format<acpi::AmlTermType>;

void AmlNameFormat::format(km::IOutStream& out, const acpi::AmlName& value) {
    if (value.segments.isEmpty()) {
        out.write("NullName");
        return;
    }

    for (size_t i = 0; i < value.prefix; i++) {
        out.write('^');
    }

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
    case acpi::AmlTermType::eUnary:
        out.write("Unary");
        break;
    case acpi::AmlTermType::eBinary:
        out.write("Binary");
        break;
    case acpi::AmlTermType::eWhile:
        out.write("While");
        break;
    case acpi::AmlTermType::eReturn:
        out.write("Return");
        break;
    case acpi::AmlTermType::eMethodInvoke:
        out.write("MethodInvoke");
        break;
    default:
        out.write(std::to_underlying(value));
    }
}
