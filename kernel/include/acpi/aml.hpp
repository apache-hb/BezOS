#pragma once

#include "acpi/header.hpp"

#include "allocator/bitmap.hpp"
#include "allocator/bump.hpp"

#include "std/memory.hpp"
#include "std/static_vector.hpp"
#include "std/vector.hpp"
#include "util/digit.hpp"

#include <span>

#include <stdint.h>

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

    struct AmlData {
        enum class Type {
            eByte,
            eWord,
            eDword,
            eQword,
            eString,
            ePackage,
        } type;

        union Data {
            Data() { }
            Data(uint64_t i)
                : integer(i)
            { }

            Data(stdx::StringView s)
                : string(s)
            { }

            Data(AmlPackageId p)
                : package(p)
            { }

            uint64_t integer;
            stdx::StringView string;
            AmlPackageId package;
        } data;

        AmlData() { }

        AmlData(Type ty, Data d)
            : type(ty)
            , data(d)
        { }

        bool isInteger() const { return type == Type::eByte || type == Type::eWord || type == Type::eDword || type == Type::eQword; }
        bool isString() const { return type == Type::eString; }
        bool isPackage() const { return type == Type::ePackage; }
    };

    enum class AmlTermType : uint8_t {
        eName,
        eMethod,
        ePackage,
        eData,
        eOpRegion,
    };

    struct AmlNameTerm {
        AmlName name;
        AmlAnyId data;
    };

    struct AmlMethodTerm {
        AmlName name;
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

    struct AmlOpRegionTerm {
        AmlName name;
        AddressSpaceId region;
        AmlData offset;
        AmlData size;
    };

    class AmlNodeBuffer {
        struct ObjectHeader {
            AmlTermType type;
            AmlOffsetType offset;
        };

        AmlAllocator mAllocator;

        stdx::Vector<ObjectHeader, mem::AllocatorPointer<AmlAllocator>> mHeaders;
        stdx::MemoryResource<mem::AllocatorPointer<AmlAllocator>> mObjects;

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
        AmlNodeBuffer(AmlAllocator allocator, RsdtHeader header)
            : mAllocator(allocator)
            , mHeaders(&mAllocator)
            , mObjects(&mAllocator)
            , mOnes(AmlNodeBuffer::ones(header.revision))
        { }

        AmlAllocator *allocator() { return &mAllocator; }

        AmlNameId add(AmlNameTerm term);
        AmlMethodId add(AmlMethodTerm term);
        AmlPackageId add(AmlPackageTerm term);
        AmlDataId add(AmlDataTerm term);
        AmlOpRegionId add(AmlOpRegionTerm term);

        AmlData ones() { return mOnes; }

        AmlIdRange addRange(auto&& fn) {
            AmlAnyId front = { mHeaders.count() };
            fn();
            AmlAnyId back = { mHeaders.count() };

            return AmlIdRange { front, back };
        }
    };

    class AmlCode {
        RsdtHeader mHeader;

        AmlNodeBuffer mNodes;

    public:
        AmlCode(RsdtHeader header, AmlAllocator arena)
            : mHeader(header)
            , mNodes(arena, mHeader)
        { }

        AmlNodeBuffer& nodes() { return mNodes; }
    };

    AmlCode WalkAml(const RsdtHeader *dsdt, AmlAllocator arena);

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
    }

    namespace literals {
        constexpr AmlName operator""_aml(const char *str, size_t len) {
            size_t prefix = 0;
            while (*str == '\\') {
                prefix++;
                str++;
            }

            AmlName name;
            name.prefix = prefix;

            while (len >= 4) {
                acpi::AmlName::Segment seg;

                // TODO: fixup case where \\_PR -> \\_PR_
                seg.add(str, str + 4);

                name.segments.add(seg);

                len -= 4;

                if (len > 0 && *str == '.') {
                    str++;
                    len--;
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
