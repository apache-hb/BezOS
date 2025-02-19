#pragma once

#include "acpi/header.hpp"

#include "allocator/bitmap.hpp"

#include "log.hpp"
#include "std/memory.hpp"
#include "std/static_vector.hpp"
#include "std/vector.hpp"

#include <expected>
#include <span>

#include <stdint.h>
#include <variant>

namespace acpi {
    using AmlOffsetType = uint32_t;
    using AmlAllocator = mem::BitmapAllocator;

    struct DeviceHandle {

    };

    class AmlParser {
        uint32_t mStart;
        uint32_t mOffset;
        std::span<const uint8_t> mCode;

    public:
        AmlParser(std::span<const uint8_t> code, uint32_t start = 0)
            : mStart(start)
            , mOffset(0)
            , mCode(code)
        { }

        AmlParser(const RsdtHeader *header);

        uint32_t offset() const { return mOffset; }
        uint32_t absOffset() const { return mStart + mOffset; }
        const char *current() const { return (const char*)mCode.data() + mOffset; }

        void skip(uint32_t count) { mOffset += count; }
        void advance() { skip(1); }

        uint8_t peek() const;
        uint8_t read();

        bool eof() const { return mOffset >= mCode.size(); }

        template<typename F>
        bool match(F&& fn) {
            if (fn(peek())) {
                advance();
                return true;
            }

            return false;
        }

        bool consume(uint8_t v) {
            return match([v](uint8_t c) { return c == v; });
        }

        template<typename... A>
        bool consumeSeq(A&&... args) {
            std::array<uint8_t, sizeof...(args)> seq = { static_cast<uint8_t>(args)... };

            if (mOffset + seq.size() > mCode.size()) {
                return false;
            }

            bool matched = memcmp(mCode.data() + mOffset, seq.data(), seq.size()) == 0;
            if (matched) {
                mOffset += seq.size();
            }

            return matched;
        }

        bool done() const {
            return mOffset >= mCode.size();
        }

        AmlParser subspan(uint32_t count) {
            uint32_t offset = mOffset;
            mOffset += count;
            return { mCode.subspan(offset, count), offset + mStart };
        }
    };

    struct AmlName {
        using Segment = stdx::StaticString<4>;
        uint8_t prefix = 0;
        bool useroot = false;
        stdx::StaticVector<Segment, 8> segments;

        constexpr bool operator==(const AmlName& other) const = default;
    };

    struct AmlAnyId {
        AmlOffsetType id;

        static const AmlAnyId kInvalid;

        constexpr bool operator==(const AmlAnyId& other) const = default;
        constexpr operator bool() const { return id != kInvalid.id; }
    };

    constexpr AmlAnyId AmlAnyId::kInvalid = { 0xFFFF'FFFF };

    template<typename T>
    struct AmlId : public AmlAnyId {
        AmlId(AmlOffsetType id)
            : AmlAnyId(id)
        { }

        AmlId(AmlAnyId id)
            : AmlAnyId(id)
        { }

        constexpr bool operator==(const AmlId& other) const = default;
        constexpr operator bool() const { return id != kInvalid.id; }
    };

    using AmlNameId = AmlId<struct AmlNameTerm>;
    using AmlMethodId = AmlId<struct AmlMethodTerm>;
    using AmlPackageId = AmlId<struct AmlPackageTerm>;
    using AmlDataId = AmlId<struct AmlDataTerm>;
    using AmlOpRegionId = AmlId<struct AmlOpRegionTerm>;
    using AmlScopeId = AmlId<struct AmlScopeTerm>;
    using AmlAliasId = AmlId<struct AmlAliasTerm>;
    using AmlDeviceId = AmlId<struct AmlDeviceTerm>;
    using AmlProcessorId = AmlId<struct AmlProcessorTerm>;
    using AmlIfElseId = AmlId<struct IfElseOp>;
    using AmlCondRefOfId = AmlId<struct CondRefOp>;

    enum class AmlArgObject : uint8_t {
        eArg0 = 0x68,
        eArg1 = 0x69,
        eArg2 = 0x6A,
        eArg3 = 0x6B,
        eArg4 = 0x6C,
        eArg5 = 0x6D,
        eArg6 = 0x6E,
    };

    enum AmlLocalObject : uint8_t {
        eLocal0 = 0x60,
        eLocal1 = 0x61,
        eLocal2 = 0x62,
        eLocal3 = 0x63,
        eLocal4 = 0x64,
        eLocal5 = 0x65,
        eLocal6 = 0x66,
        eLocal7 = 0x67,
    };

    struct AmlBuffer {
        acpi::AmlAnyId size;
        stdx::Vector<uint8_t> data;
    };

    struct AmlData {
        enum class Type {
            eByte,
            eWord,
            eDword,
            eQword,
            eString,
            eBuffer,
            ePackage,
            eArg,
            eLocal,
        } type;

        using Data = std::variant<
            uint64_t,
            stdx::StringView,
            AmlPackageId,
            AmlBuffer,
            AmlArgObject,
            AmlLocalObject
        >;

        Data data;

        constexpr AmlData() { }

        constexpr AmlData(Type ty, Data d)
            : type(ty)
            , data(std::move(d))
        { }

        bool isInteger() const { return type == Type::eByte || type == Type::eWord || type == Type::eDword || type == Type::eQword; }
        bool isString() const { return type == Type::eString; }
        bool isPackage() const { return type == Type::ePackage; }
    };

    struct AmlNullName { };
    struct AmlDebugObject { };

    using AmlSimpleName = std::variant<AmlName, AmlArgObject, AmlLocalObject>;

    using AmlSuperName = std::variant<AmlName, AmlArgObject, AmlLocalObject, AmlAnyId, AmlDebugObject>;

    using AmlTarget = std::variant<AmlName, AmlArgObject, AmlLocalObject, AmlAnyId, AmlNullName, AmlDebugObject>;

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

    struct AmlFieldFlags {
        uint8_t flags;

        bool lock() const {
            return flags & (1 << 4);
        }
    };

    enum class AmlTermType : uint8_t {
        eName,
        eMethod,
        ePackage,
        eData,
        eStore,
        eOpRegion,
        eMutex,
        eScope,
        eAlias,
        eDevice,
        eProcessor,
        eIfElse,
        eElse,
        eWhile,
        eReturn,
        eCondRefOf,
        eSizeOf,
        eField,
        eMethodInvoke,
        eIndexField,
        eExternal,
        ePowerRes,
        eThermalZone,

        eCreateByteField,
        eCreateWordField,
        eCreateDwordField,
        eCreateQwordField,

        eUnary,
        eExpression,
        eBinary,
        eBinaryTac,
        eUnaryTac,
        eToString,
        eStatement,
        eFatal,
        eDivide,
        eWait,

        eAcquire,
        eRelease,
        eNotify,

        eInvalid,
    };

    using AmlTermList = stdx::Vector<AmlAnyId>;

    struct AmlStoreTerm {
        static constexpr AmlTermType kType = AmlTermType::eStore;
        AmlAnyId source;
        AmlSuperName target;
    };

    struct AmlNameTerm {
        static constexpr AmlTermType kType = AmlTermType::eName;
        AmlName name;
        AmlAnyId data;
    };

    struct AmlMethodTerm {
        static constexpr AmlTermType kType = AmlTermType::eMethod;
        AmlName name;
        AmlMethodFlags flags;
        AmlTermList terms;
    };

    struct AmlThermalZoneTerm {
        static constexpr AmlTermType kType = AmlTermType::eThermalZone;
        AmlName name;
        AmlTermList terms;
    };

    struct AmlNameString {
        static constexpr AmlTermType kType = AmlTermType::eName;
        AmlName name;
    };

    struct AmlPackageTerm {
        static constexpr AmlTermType kType = AmlTermType::ePackage;
        uint8_t count;
        AmlTermList terms;
    };

    struct AmlDataTerm {
        static constexpr AmlTermType kType = AmlTermType::eData;
        AmlData data;
    };

    struct AmlAliasTerm {
        static constexpr AmlTermType kType = AmlTermType::eAlias;
        AmlName name;
        AmlName alias;
    };

    struct AmlDeviceTerm {
        static constexpr AmlTermType kType = AmlTermType::eDevice;
        AmlName name;
        AmlTermList terms;
    };

    struct AmlFieldTerm {
        static constexpr AmlTermType kType = AmlTermType::eField;
        AmlName name;
        AmlFieldFlags flags;
    };

    struct AmlMethodInvokeTerm {
        static constexpr AmlTermType kType = AmlTermType::eMethodInvoke;
        AmlName name;
        AmlTermList args;
    };

    struct AmlIndexFieldTerm {
        static constexpr AmlTermType kType = AmlTermType::eIndexField;
        AmlName name;
        AmlName field;
        AmlFieldFlags flags;
    };

    struct AmlPowerResTerm {
        static constexpr AmlTermType kType = AmlTermType::ePowerRes;
        AmlName name;
        uint8_t level;
        uint16_t order;
        AmlTermList terms;
    };

    struct AmlOpRegionTerm {
        static constexpr AmlTermType kType = AmlTermType::eOpRegion;
        AmlName name;
        AddressSpaceId region;
        AmlAnyId offset;
        AmlAnyId size;
    };

    struct AmlMutexTerm {
        static constexpr AmlTermType kType = AmlTermType::eMutex;
        AmlName name;
        uint8_t flags;
    };

    struct AmlScopeTerm {
        static constexpr AmlTermType kType = AmlTermType::eScope;
        AmlName name;
        AmlTermList terms;
    };

    struct AmlProcessorTerm {
        static constexpr AmlTermType kType = AmlTermType::eProcessor;
        AmlName name;
        uint8_t id;
        uint32_t pblkAddr;
        uint8_t pblkLen;
        AmlTermList terms;
    };

    struct AmlBranchTerm {
        static constexpr AmlTermType kType = AmlTermType::eIfElse;
        AmlAnyId predicate;
        AmlTermList terms;
        AmlAnyId otherwise;
    };

    struct AmlElseTerm {
        static constexpr AmlTermType kType = AmlTermType::eElse;
        AmlTermList terms;
    };

    struct AmlWhileTerm {
        static constexpr AmlTermType kType = AmlTermType::eWhile;
        AmlAnyId predicate;
        AmlTermList terms;
    };

    struct AmlReturnTerm {
        static constexpr AmlTermType kType = AmlTermType::eReturn;
        AmlAnyId value;
    };

    struct AmlCondRefOfTerm {
        static constexpr AmlTermType kType = AmlTermType::eCondRefOf;
        AmlSuperName name;
        AmlTarget target;
    };

    struct AmlSizeOfTerm {
        static constexpr AmlTermType kType = AmlTermType::eSizeOf;
        AmlSuperName term;
    };

    struct AmlCreateByteFieldTerm {
        static constexpr AmlTermType kType = AmlTermType::eCreateByteField;
        AmlAnyId source;
        AmlAnyId index;
        AmlName name;
    };

    struct AmlCreateWordFieldTerm {
        static constexpr AmlTermType kType = AmlTermType::eCreateWordField;
        AmlAnyId source;
        AmlAnyId index;
        AmlName name;
    };

    struct AmlCreateDwordFieldTerm {
        static constexpr AmlTermType kType = AmlTermType::eCreateDwordField;
        AmlAnyId source;
        AmlAnyId index;
        AmlName name;
    };

    struct AmlCreateQwordFieldTerm {
        static constexpr AmlTermType kType = AmlTermType::eCreateQwordField;
        AmlAnyId source;
        AmlAnyId index;
        AmlName name;
    };

    struct AmlExternalTerm {
        static constexpr AmlTermType kType = AmlTermType::eExternal;
        AmlName name;
        uint8_t type;
        uint8_t args;
    };

    struct AmlUnaryTerm {
        static constexpr AmlTermType kType = AmlTermType::eUnary;
        enum Type {
            eNot,
            eDerefOf,
        } type;
        AmlAnyId term;
    };

    struct AmlExpression {
        static constexpr AmlTermType kType = AmlTermType::eExpression;
        enum Type {
            eIncrement,
            eDecrement,
        } type;
        AmlSuperName term;
    };

    struct AmlStatementTerm {
        static constexpr AmlTermType kType = AmlTermType::eStatement;
        enum Type {
            eContinue,
            eBreak,
            eBreakPoint,
        } type;
    };

    struct AmlFatalTerm {
        static constexpr AmlTermType kType = AmlTermType::eFatal;
        uint8_t type;
        uint32_t code;
        AmlAnyId arg;
    };

    struct AmlBinaryTerm {
        static constexpr AmlTermType kType = AmlTermType::eBinary;
        enum Type {
            eEqual,
            eNotEqual,
            eLess,
            eLessEqual,
            eGreater,
            eGreaterEqual,
            eOr,
            eAnd,
            eAdd,
            eSub,
            eMul,
            eDiv,
        } type;
        AmlAnyId left;
        AmlAnyId right;
    };

    struct AmlBinaryTacTerm {
        static constexpr AmlTermType kType = AmlTermType::eBinaryTac;
        enum Type {
            eSub,
            eAdd,
            eMul,
            eIndex,
            eAnd,
            eOr,
            eShiftLeft,
            eShiftRight,
        } type;
        AmlAnyId left;
        AmlAnyId right;
        AmlTarget target;
    };

    struct AmlUnaryTacTerm {
        static constexpr AmlTermType kType = AmlTermType::eUnaryTac;
        enum Type {
            eFindSetLeftBit,
            eFindSetRightBit,
            eToHexString,
            eToBuffer,
            eToInteger,
            eToString,
        } type;
        AmlAnyId term;
        AmlTarget target;
    };

    struct AmlToStringTerm {
        static constexpr AmlTermType kType = AmlTermType::eToString;
        AmlAnyId term;
        AmlAnyId length;
        AmlTarget target;
    };

    struct AmlDivideTerm {
        static constexpr AmlTermType kType = AmlTermType::eDivide;
        AmlAnyId dividend;
        AmlAnyId divisor;
        AmlTarget remainder;
        AmlTarget quotient;
    };

    struct AmlWaitTerm {
        static constexpr AmlTermType kType = AmlTermType::eWait;
        enum Type {
            eStall,
            eSleep,
        } type;
        AmlAnyId value;
    };

    struct AmlAcquireTerm {
        static constexpr AmlTermType kType = AmlTermType::eAcquire;
        AmlSuperName name;
        uint16_t timeout;
    };

    struct AmlReleaseTerm {
        static constexpr AmlTermType kType = AmlTermType::eRelease;
        AmlSuperName name;
    };

    struct AmlNotifyTerm {
        static constexpr AmlTermType kType = AmlTermType::eNotify;
        AmlSuperName name;
        AmlAnyId value;
    };

    class AmlNodeBuffer {
    public:
        struct ObjectHeader {
            AmlTermType type;
            AmlOffsetType offset;
        };
    private:
        mem::IAllocator *mAllocator;

        stdx::Vector<ObjectHeader> mHeaders;
        stdx::MemoryResource mObjects;

        AmlData mOnes;
        bool mError;

        static AmlData ones(uint32_t revision) {
            if (revision == 0) {
                return { AmlData::Type::eDword, { 0xFFFF'FFFF } };
            } else {
                return { AmlData::Type::eQword, { 0xFFFF'FFFF'FFFF'FFFF } };
            }
        }

        template<typename T>
        AmlId<T> addTerm(T term, AmlTermType type) {
            size_t offset = mObjects.add<T>(std::move(term));

            size_t index = mHeaders.count();
            mHeaders.add(ObjectHeader { type, uint32_t(offset) });
            return { uint32_t(index) };
        }

    public:
        AmlNodeBuffer(mem::IAllocator *allocator, uint32_t revision)
            : mAllocator(allocator)
            , mHeaders(mAllocator)
            , mObjects(mAllocator)
            , mOnes(AmlNodeBuffer::ones(revision))
            , mError(false)
        { }

        mem::IAllocator *allocator() { return mAllocator; }

        template<typename T>
        AmlId<T> add(T term) { return addTerm(term, T::kType); }

        template<typename T>
        T *getTerm(AmlAnyId id) {
            ObjectHeader header = getHeader(id);
            if (header.type != T::kType) {
                return nullptr;
            }

            return mObjects.get<T>(header.offset);
        }

        template<typename T>
        T *get(AmlId<T> id) { return getTerm<T>(id); }

        AmlTermType getType(AmlAnyId id) const { return getHeader(id).type; }
        ObjectHeader getHeader(AmlAnyId id) const {
            if (id.id >= mHeaders.count()) {
                return { AmlTermType::eInvalid, 0 };
            }

            return mHeaders[id.id];
        }

        AmlData ones() { return mOnes; }

        void setInvalid() { mError = true; }
        bool error() const { return mError; }
    };

    enum class AmlError {
        eOk
    };

    class AmlCode {
        RsdtHeader mHeader;

        AmlNodeBuffer mNodes;

        AmlScopeId mRootScope;

    public:
        AmlCode(RsdtHeader header, mem::IAllocator *arena)
            : mHeader(header)
            , mNodes(arena, mHeader.revision)
            , mRootScope(mNodes.add(AmlScopeTerm { .terms = AmlTermList(mNodes.allocator()) }))
        { }

        AmlNodeBuffer& nodes() { return mNodes; }

        std::expected<uint64_t, AmlError> intValue(AmlName name) const;

        AmlScopeTerm *root() { return mNodes.get<AmlScopeTerm>(mRootScope); }
        AmlScopeId rootId() { return mRootScope; }

        DeviceHandle findDevice(stdx::StringView) { return DeviceHandle {}; }
    };

    AmlCode WalkAml(const RsdtHeader *dsdt, mem::IAllocator *arena);

    namespace detail {
        // 20.2.3. Data Objects Encoding
        uint8_t ByteData(AmlParser& parser);
        uint16_t WordData(AmlParser& parser);
        uint32_t DwordData(AmlParser& parser);
        uint64_t QwordData(AmlParser& parser);

        // 20.2.4. Package Length Encoding
        uint32_t PkgLength(AmlParser& parser);

        // 20.2.2. Name Objects Encoding
        AmlName NameString(AmlParser& parser, AmlNodeBuffer& code);

        AmlSuperName SuperName(AmlParser& parser, AmlNodeBuffer& code);

        AmlTarget Target(AmlParser& parser, AmlNodeBuffer& code);

        AmlAnyId DataRefObject(AmlParser& parser, AmlNodeBuffer& code);

        AmlNameTerm NameTerm(AmlParser& parser, AmlNodeBuffer& code);

        AmlScopeTerm ScopeTerm(AmlParser& parser, AmlNodeBuffer& code);

        AmlMethodTerm MethodTerm(AmlParser& parser, AmlNodeBuffer& code);

        AmlAnyId Term(AmlParser& parser, AmlNodeBuffer& code);

        AmlAnyId ExpressionOp(AmlParser& parser, AmlNodeBuffer& code);

        AmlAnyId TermArg(AmlParser& parser, AmlNodeBuffer& code);

        AmlAliasTerm AliasTerm(AmlParser& parser, AmlNodeBuffer& code);

        AmlDeviceTerm DeviceTerm(AmlParser& parser, AmlNodeBuffer& code);

        AmlProcessorTerm ProcessorTerm(AmlParser& parser, AmlNodeBuffer& code);

        AmlBuffer BufferData(AmlParser& parser, AmlNodeBuffer& code);

        AmlBranchTerm DefIfElse(AmlParser& parser, AmlNodeBuffer& code);

        AmlElseTerm DefElse(AmlParser& parser, AmlNodeBuffer& code);

        AmlReturnTerm DefReturn(AmlParser& parser, AmlNodeBuffer& code);

        AmlWhileTerm DefWhile(AmlParser& parser, AmlNodeBuffer& code);

        AmlMethodInvokeTerm DefMethodInvoke(AmlParser& parser, AmlNodeBuffer& code);

        AmlCondRefOfTerm ConfRefOf(AmlParser& parser, AmlNodeBuffer& code);

        AmlOpRegionTerm DefOpRegion(AmlParser& parser, AmlNodeBuffer& code);

        AmlMutexTerm DefMutex(AmlParser& parser, AmlNodeBuffer& code);

        AmlStoreTerm DefStore(AmlParser& parser, AmlNodeBuffer& code);

        AmlFieldTerm DefField(AmlParser& parser, AmlNodeBuffer& code);

        AmlThermalZoneTerm DefThermalZone(AmlParser& parser, AmlNodeBuffer& code);

        AmlIndexFieldTerm DefIndexField(AmlParser& parser, AmlNodeBuffer& code);

        AmlPowerResTerm DefPowerRes(AmlParser& parser, AmlNodeBuffer& code);

        AmlExternalTerm DefExternal(AmlParser& parser, AmlNodeBuffer& code);

        AmlCreateByteFieldTerm DefCreateByteField(AmlParser& parser, AmlNodeBuffer& code);
        AmlCreateWordFieldTerm DefCreateWordField(AmlParser& parser, AmlNodeBuffer& code);
        AmlCreateDwordFieldTerm DefCreateDwordField(AmlParser& parser, AmlNodeBuffer& code);
        AmlCreateQwordFieldTerm DefCreateQwordField(AmlParser& parser, AmlNodeBuffer& code);

        AmlUnaryTerm DefLNot(AmlParser& parser, AmlNodeBuffer& code);
        AmlBinaryTerm DefLEqual(AmlParser& parser, AmlNodeBuffer& code);
        AmlBinaryTerm DefLOr(AmlParser& parser, AmlNodeBuffer& code);
        AmlBinaryTerm DefLAnd(AmlParser& parser, AmlNodeBuffer& code);
        AmlBinaryTerm DefLLess(AmlParser& parser, AmlNodeBuffer& code);
        AmlBinaryTerm DefLGreater(AmlParser& parser, AmlNodeBuffer& code);

        AmlSizeOfTerm DefSizeOf(AmlParser& parser, AmlNodeBuffer& code);
        AmlExpression DefDecrement(AmlParser& parser, AmlNodeBuffer& code);
        AmlExpression DefIncrement(AmlParser& parser, AmlNodeBuffer& code);
        AmlUnaryTerm DefDerefOf(AmlParser& parser, AmlNodeBuffer& code);

        AmlBinaryTacTerm DefIndex(AmlParser& parser, AmlNodeBuffer& code);
        AmlBinaryTacTerm DefSubtract(AmlParser& parser, AmlNodeBuffer& code);
        AmlBinaryTacTerm DefAdd(AmlParser& parser, AmlNodeBuffer& code);
        AmlBinaryTacTerm DefAnd(AmlParser& parser, AmlNodeBuffer& code);
        AmlBinaryTacTerm DefOr(AmlParser& parser, AmlNodeBuffer& code);
        AmlBinaryTacTerm DefMultiply(AmlParser& parser, AmlNodeBuffer& code);
        AmlBinaryTacTerm DefShiftLeft(AmlParser& parser, AmlNodeBuffer& code);
        AmlBinaryTacTerm DefShiftRight(AmlParser& parser, AmlNodeBuffer& code);

        AmlAcquireTerm DefAcquire(AmlParser& parser, AmlNodeBuffer& code);
        AmlReleaseTerm DefRelease(AmlParser& parser, AmlNodeBuffer& code);
        AmlNotifyTerm DefNotify(AmlParser& parser, AmlNodeBuffer& code);
        AmlStatementTerm DefContinue(AmlParser& parser, AmlNodeBuffer& code);
        AmlStatementTerm DefBreak(AmlParser& parser, AmlNodeBuffer& code);
        AmlStatementTerm DefBreakPoint(AmlParser& parser, AmlNodeBuffer& code);
        AmlFatalTerm DefFatal(AmlParser& parser, AmlNodeBuffer& code);

        AmlUnaryTacTerm DefFindSetLeftBit(AmlParser& parser, AmlNodeBuffer& code);
        AmlUnaryTacTerm DefFindSetRightBit(AmlParser& parser, AmlNodeBuffer& code);
        AmlUnaryTacTerm DefToHexString(AmlParser& parser, AmlNodeBuffer& code);
        AmlUnaryTacTerm DefToBuffer(AmlParser& parser, AmlNodeBuffer& code);
        AmlUnaryTacTerm DefToInteger(AmlParser& parser, AmlNodeBuffer& code);
        AmlToStringTerm DefToString(AmlParser& parser, AmlNodeBuffer& code);

        AmlWaitTerm DefSleep(AmlParser& parser, AmlNodeBuffer& code);
        AmlWaitTerm DefStall(AmlParser& parser, AmlNodeBuffer& code);

        AmlDivideTerm DefDivide(AmlParser& parser, AmlNodeBuffer& code);

        AmlTermList TermList(AmlParser& parser, AmlNodeBuffer& code);

        template<typename... A>
        void ReportError(AmlParser& parser, AmlNodeBuffer& code, A&&... args) {
            code.setInvalid();
            KmDebugMessage("[AML] ", std::forward<A>(args)..., "\n");

            if (parser.eof()) {
                KmDebugMessage("[AML] Reached end of code (", parser.absOffset(), ")\n");
            } else {
                KmDebugMessage("[AML] At offset ", parser.absOffset(), " (local offset: ", parser.offset(), ")\n");
            }
        }
    }

    namespace literals {
        constexpr AmlName operator""_aml(const char *str, size_t len) {
            const char *end = str + len;

            AmlName name;
            if (*str == '\\') {
                str += 1;
                name.useroot = true;
            } else {
                while (*str == '^') {
                    name.prefix += 1;
                    str++;
                }
            }

            // chunk the name into 4 byte segments seperated with '.'
            // if the '.' appears within the name segment then pad with
            // trailing '_'

            while (str < end) {
                char segment[4];
                for (size_t i = 0; i < 4; i++) {
                    if (str < end) {
                        if (*str == '.') {
                            str++;
                            // fill rest of segment with '_'
                            for (; i < 4; i++) {
                                segment[i] = '_';
                            }
                            break;
                        }

                        segment[i] = *str++;
                    } else {
                        segment[i] = '_';
                    }
                }

                name.segments.add(segment);

                if (*str == '.') {
                    str++;
                }
            }

            return name;
        }
    }
}

template<>
struct km::Format<acpi::AmlName> {
    static void format(km::IOutStream& out, const acpi::AmlName& value);
};

template<>
struct km::Format<acpi::AmlData> {
    static void format(km::IOutStream& out, const acpi::AmlData& value);
};

template<>
struct km::Format<acpi::AmlTermType> {
    static void format(km::IOutStream& out, acpi::AmlTermType value);
};
