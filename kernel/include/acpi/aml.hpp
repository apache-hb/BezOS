#pragma once

#include "acpi/header.hpp"

#include "allocator/bitmap.hpp"
#include "allocator/bump.hpp"

#include "std/memory.hpp"
#include "std/static_vector.hpp"
#include "std/vector.hpp"
#include "util/digit.hpp"

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

        bool consume(uint8_t v) {
            if (peek() == v) {
                advance();
                return true;
            }

            return false;
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
        size_t prefix = 0;
        stdx::StaticVector<Segment, 8> segments;

        constexpr bool operator==(const AmlName& other) const = default;
    };

    struct AmlAnyId {
        AmlOffsetType id;

        constexpr bool operator==(const AmlAnyId& other) const = default;
    };

    template<typename T>
    struct AmlId : public AmlAnyId {
        AmlId(AmlOffsetType id)
            : AmlAnyId(id)
        { }

        AmlId(AmlAnyId id)
            : AmlAnyId(id)
        { }
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

    struct AmlTermArg {
        std::variant<
            AmlAnyId,
            AmlArgObject,
            AmlLocalObject
        > data;
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

        AmlData() { }

        AmlData(Type ty, Data d)
            : type(ty)
            , data(std::move(d))
        { }

        bool isInteger() const { return type == Type::eByte || type == Type::eWord || type == Type::eDword || type == Type::eQword; }
        bool isString() const { return type == Type::eString; }
        bool isPackage() const { return type == Type::ePackage; }
    };

    struct AmlExpr {

    };

    struct AmlSuperName {
        AmlName name;
    };

    struct AmlTarget {
        acpi::AmlSuperName name;
    };

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
        eCondRefOf,
        eField,
        eIndexField,
        eExternal,
        ePowerRes,
        eThermalZone,

        eCreateByteField,
        eCreateWordField,
        eCreateDwordField,
        eCreateQwordField,
    };

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
        stdx::Vector<AmlAnyId> terms;
    };

    struct AmlThermalZoneTerm {
        static constexpr AmlTermType kType = AmlTermType::eThermalZone;
        AmlName name;
        stdx::Vector<AmlAnyId> terms;
    };

    struct AmlNameString {
        static constexpr AmlTermType kType = AmlTermType::eName;
        AmlName name;
    };

    struct AmlPackageTerm {
        static constexpr AmlTermType kType = AmlTermType::ePackage;
        stdx::Vector<AmlAnyId> terms;
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
        stdx::Vector<AmlAnyId> terms;
    };

    struct AmlFieldTerm {
        static constexpr AmlTermType kType = AmlTermType::eField;
        AmlName name;
        AmlFieldFlags flags;
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
        stdx::Vector<AmlAnyId> terms;
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
        stdx::Vector<AmlAnyId> terms;
    };

    struct AmlProcessorTerm {
        static constexpr AmlTermType kType = AmlTermType::eProcessor;
        AmlName name;
        uint8_t id;
        uint32_t pblkAddr;
        uint8_t pblkLen;
    };

    struct AmlIfElseOp {
        static constexpr AmlTermType kType = AmlTermType::eIfElse;
        AmlAnyId predicate;
        stdx::Vector<AmlAnyId> terms;
    };

    struct AmlCondRefOp {
        static constexpr AmlTermType kType = AmlTermType::eCondRefOf;
        AmlSuperName name;
        AmlTarget target;
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
        { }

        mem::IAllocator *allocator() { return mAllocator; }

        template<typename T>
        AmlId<T> add(T term) { return addTerm(term, T::kType); }

        template<typename T>
        T *getTerm(AmlAnyId id) {
            return mObjects.get<T>(getHeader(id).offset);
        }

        template<typename T>
        T *get(AmlId<T> id) { return getTerm<T>(id); }

        AmlTermType getType(AmlAnyId id) const { return getHeader(id).type; }
        ObjectHeader getHeader(AmlAnyId id) const { return mHeaders[id.id]; }

        AmlData ones() { return mOnes; }
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
            , mRootScope(mNodes.add(AmlScopeTerm { .terms = stdx::Vector<AmlAnyId>(mNodes.allocator()) }))
        { }

        AmlNodeBuffer& nodes() { return mNodes; }

        std::expected<uint64_t, AmlError> intValue(AmlName name) const;

        AmlScopeTerm *root() { return mNodes.get<AmlScopeTerm>(mRootScope); }
        AmlScopeId rootId() { return mRootScope; }

        DeviceHandle findDevice(stdx::StringView hid) { return DeviceHandle {}; }
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
        AmlName NameString(AmlParser& parser, uint8_t lead = 0);

        AmlSuperName SuperName(AmlParser& parser);

        AmlTarget Target(AmlParser& parser);

        AmlData DataRefObject(AmlParser& parser, AmlNodeBuffer& code);

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

        AmlIfElseOp DefIfElse(AmlParser& parser, AmlNodeBuffer& code);

        AmlCondRefOp ConfRefOf(AmlParser& parser, AmlNodeBuffer& code);

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
    }

    namespace literals {
        constexpr AmlName operator""_aml(const char *str, size_t len) {
            const char *end = str + len;
            size_t prefix = 0;
            while (*str == '^') {
                prefix++;
                str++;
            }

            AmlName name;
            name.prefix = prefix;

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
