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
    using AmlOffsetType = sm::uint24_t;
    using AmlAllocator = mem::BitmapAllocator;

    class AmlParser {
        uint32_t mOffset;
        std::span<const uint8_t> mCode;

    public:
        AmlParser(std::span<const uint8_t> code)
            : mOffset(0)
            , mCode(code)
        { }

        AmlParser(const RsdtHeader *header);

        uint32_t offset() const { return mOffset; }
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
            return { mCode.subspan(offset, count) };
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

    struct AmlIdRange {
        AmlAnyId front;
        AmlAnyId back;

        size_t count() const { return back.id - front.id; }
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

    struct AmlTermArg {

    };

    struct AmlBuffer {
        acpi::AmlTermArg size;
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
        } type;

        using Data = std::variant<
            uint64_t,
            stdx::StringView,
            AmlPackageId,
            AmlBuffer
        >;

        Data data;

        AmlData() { }

        AmlData(Type ty, Data d)
            : type(ty)
            , data(d)
        { }

        bool isInteger() const { return type == Type::eByte || type == Type::eWord || type == Type::eDword || type == Type::eQword; }
        bool isString() const { return type == Type::eString; }
        bool isPackage() const { return type == Type::ePackage; }
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
        eOpRegion,
        eScope,
        eAlias,
        eDevice,
        eProcessor,
    };

    struct AmlNameTerm {
        AmlName name;
        AmlAnyId data;
    };

    struct AmlMethodTerm {
        AmlName name;
        AmlMethodFlags flags;
    };

    struct AmlNameString {
        AmlName name;
    };

    struct AmlPackageTerm {
        AmlIdRange range;
    };

    struct AmlDataTerm {
        AmlData data;
    };

    struct AmlAliasTerm {
        AmlName name;
        AmlName alias;
    };

    struct AmlDeviceTerm {
        AmlName name;
    };

    struct AmlOpRegionTerm {
        AmlName name;
        AddressSpaceId region;
        AmlData offset;
        AmlData size;
    };

    struct AmlScopeTerm {
        AmlName name;
        stdx::Vector<AmlAnyId> terms;
    };

    struct AmlProcessorTerm {
        AmlName name;
        uint8_t id;
        uint32_t pblkAddr;
        uint8_t pblkLen;
    };

    class AmlNodeBuffer {
        struct ObjectHeader {
            AmlTermType type;
            AmlOffsetType offset;
        };

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
            size_t offset = mObjects.add(term);

            size_t index = mHeaders.count();
            mHeaders.add(ObjectHeader { type, offset });
            return { index };
        }

    public:
        AmlNodeBuffer(mem::IAllocator *allocator, uint32_t revision)
            : mAllocator(allocator)
            , mHeaders(mAllocator)
            , mObjects(mAllocator)
            , mOnes(AmlNodeBuffer::ones(revision))
        { }

        mem::IAllocator *allocator() { return mAllocator; }

        AmlNameId add(AmlNameTerm term);
        AmlMethodId add(AmlMethodTerm term);
        AmlPackageId add(AmlPackageTerm term);
        AmlDataId add(AmlDataTerm term);
        AmlOpRegionId add(AmlOpRegionTerm term);
        AmlScopeId add(AmlScopeTerm term);
        AmlAliasId add(AmlAliasTerm term);
        AmlDeviceId add(AmlDeviceTerm term);
        AmlProcessorId add(AmlProcessorTerm term);

        AmlNameTerm get(AmlNameId id);

        AmlData ones() { return mOnes; }

        AmlIdRange addRange(auto&& fn) {
            AmlAnyId front = { mHeaders.count() };
            fn();
            AmlAnyId back = { mHeaders.count() };

            return AmlIdRange { front, back };
        }
    };

    enum class AmlError {
        eOk
    };

    class AmlCode {
        RsdtHeader mHeader;

        AmlNodeBuffer mNodes;

    public:
        AmlCode(RsdtHeader header, mem::IAllocator *arena)
            : mHeader(header)
            , mNodes(arena, mHeader.revision)
        { }

        AmlNodeBuffer& nodes() { return mNodes; }

        std::expected<uint64_t, AmlError> intValue(AmlName name) const;
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
        AmlName NameString(AmlParser& parser);

        AmlData DataRefObject(AmlParser& parser, AmlNodeBuffer& code);

        AmlNameTerm NameTerm(AmlParser& parser, AmlNodeBuffer& code);

        AmlScopeTerm ScopeTerm(AmlParser& parser, AmlNodeBuffer& code);

        AmlMethodTerm MethodTerm(AmlParser& parser, AmlNodeBuffer& code);

        AmlAnyId Term(AmlParser& parser, AmlNodeBuffer& code);

        AmlTermArg TermArg(AmlParser& parser);

        AmlAliasTerm AliasTerm(AmlParser& parser, AmlNodeBuffer& code);

        AmlDeviceTerm DeviceTerm(AmlParser& parser, AmlNodeBuffer& code);

        AmlProcessorTerm ProcessorTerm(AmlParser& parser, AmlNodeBuffer& code);

        AmlBuffer BufferData(AmlParser& parser, AmlNodeBuffer& code);
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
