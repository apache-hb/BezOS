#include <gtest/gtest.h>

#include "acpi/aml.hpp"

using namespace stdx::literals;
using namespace acpi::literals;

TEST(AmlDetailTest, ConsumeSeq) {
    uint8_t data[] = { 0x01, 0x02, 0x03, 0x04 };
    acpi::AmlParser parser(data);

    ASSERT_FALSE(parser.consumeSeq(0x02, 0x02, 0x03));
    ASSERT_EQ(parser.offset(), 0);

    ASSERT_TRUE(parser.consumeSeq(0x01, 0x02, 0x03));
    ASSERT_EQ(parser.offset(), 3);
}

TEST(AmlDetailTest, ConsumeSeqOne) {
    uint8_t data[] = { 0x01, 0x02, 0x03, 0x04 };
    acpi::AmlParser parser(data);

    ASSERT_FALSE(parser.consumeSeq(0x02));
    ASSERT_EQ(parser.offset(), 0);

    ASSERT_TRUE(parser.consumeSeq(0x01));
    ASSERT_EQ(parser.offset(), 1);
}

TEST(AmlDetailTest, PkgLengthEmpty) {
    uint8_t data[] = { 0x00 };
    acpi::AmlParser parser(data);
    uint32_t length = acpi::detail::PkgLength(parser);

    ASSERT_EQ(length, 0);
}

TEST(AmlDetailTest, PkgLengthZero) {
    uint8_t data[1] = { };

    // purposefully an empty span
    std::span<const uint8_t> code(std::span(data, data));

    acpi::AmlParser parser(code);
    uint32_t length = acpi::detail::PkgLength(parser);

    ASSERT_EQ(length, 0);
}

TEST(AmlDetailTest, PkgLengthOne) {
    uint8_t data[1] = { 0x01 };
    acpi::AmlParser parser(data);
    uint32_t length = acpi::detail::PkgLength(parser);

    ASSERT_EQ(length, 1);
}

TEST(AmlDetailTest, PkgLengthMax) {
    uint8_t data[4] = { 0b11001111, 0xFF, 0xFF, 0xFF };
    acpi::AmlParser parser(data);
    uint32_t length = acpi::detail::PkgLength(parser);

    ASSERT_EQ(length, 0x0FFFFFFF);
}

TEST(AmlDetailTest, ByteDataValue) {
    uint8_t data[] = { 0x42 };
    acpi::AmlParser parser(data);
    uint8_t value = acpi::detail::ByteData(parser);

    ASSERT_EQ(value, 0x42);
}

TEST(AmlDetailTest, WordDataValue) {
    uint8_t data[] = { 0xFF, 0x00 };
    acpi::AmlParser parser(data);
    uint16_t value = acpi::detail::WordData(parser);

    ASSERT_EQ(value, 0x00FF);
}

TEST(AmlDetailTest, DwordDataValue) {
    uint8_t data[] = { 0x11, 0x22, 0x33, 0x44 };
    acpi::AmlParser parser(data);
    uint32_t value = acpi::detail::DwordData(parser);

    ASSERT_EQ(value, 0x44332211);
}

TEST(AmlDetailTest, QwordDataValue) {
    uint8_t data[] = {
        0x11, 0x22, 0x33, 0x44,
        0x55, 0x66, 0x77, 0x88
    };

    acpi::AmlParser parser(data);
    uint64_t value = acpi::detail::QwordData(parser);

    ASSERT_EQ(value, 0x8877665544332211);
}

static constexpr size_t kSize = 0x10000;

static auto GetAllocatorMemory() {
    auto deleter = [](uint8_t *ptr) {
        :: operator delete[] (ptr, std::align_val_t(16));
    };

    return std::unique_ptr<uint8_t[], decltype(deleter)>{
        new (std::align_val_t(16)) uint8_t[kSize],
        deleter
    };
}

TEST(AmlDetailTest, DataRefObjectByte) {
    uint8_t data[] = { 0x0A, 0x42 };

    std::unique_ptr memory = GetAllocatorMemory();
    acpi::AmlAllocator bitmap {memory.get(), memory.get() + kSize, 16};

    acpi::AmlNodeBuffer code { &bitmap, 2 };
    acpi::AmlParser parser(data);
    acpi::AmlAnyId id = acpi::detail::DataRefObject(parser, code);
    acpi::AmlDataTerm value = *code.get<acpi::AmlDataTerm>(id);

    ASSERT_EQ(value.data.type, acpi::AmlData::Type::eByte);
    ASSERT_EQ(std::get<uint64_t>(value.data.data), 0x42);
}

TEST(AmlDetailTest, DataRefObjectWord) {
    uint8_t data[] = { 0x0B, 0xFF, 0x00 };

    std::unique_ptr memory = GetAllocatorMemory();
    acpi::AmlAllocator bitmap {memory.get(), memory.get() + kSize, 16};

    acpi::AmlNodeBuffer code { &bitmap, 2 };
    acpi::AmlParser parser(data);
    acpi::AmlAnyId id = acpi::detail::DataRefObject(parser, code);
    acpi::AmlDataTerm value = *code.get<acpi::AmlDataTerm>(id);

    ASSERT_EQ(value.data.type, acpi::AmlData::Type::eWord);
    ASSERT_EQ(std::get<uint64_t>(value.data.data), 0x00FF);
}

TEST(AmlDetailTest, DataRefObjectDword) {
    uint8_t data[] = { 0x0C, 0x11, 0x22, 0x33, 0x44 };

    std::unique_ptr memory = GetAllocatorMemory();
    acpi::AmlAllocator bitmap {memory.get(), memory.get() + kSize, 16};

    acpi::AmlNodeBuffer code { &bitmap, 2 };
    acpi::AmlParser parser(data);
    acpi::AmlAnyId id = acpi::detail::DataRefObject(parser, code);
    acpi::AmlDataTerm value = *code.get<acpi::AmlDataTerm>(id);

    ASSERT_EQ(value.data.type, acpi::AmlData::Type::eDword);
    ASSERT_EQ(std::get<uint64_t>(value.data.data), 0x44332211);
}

TEST(AmlDetailTest, DataRefObjectQword) {
    uint8_t data[] = {
        0x0E, 0x11, 0x22, 0x33, 0x44,
        0x55, 0x66, 0x77, 0x88
    };

    std::unique_ptr memory = GetAllocatorMemory();
    acpi::AmlAllocator bitmap {memory.get(), memory.get() + kSize, 16};

    acpi::AmlNodeBuffer code { &bitmap, 2 };
    acpi::AmlParser parser(data);
    acpi::AmlAnyId id = acpi::detail::DataRefObject(parser, code);
    acpi::AmlDataTerm value = *code.get<acpi::AmlDataTerm>(id);

    ASSERT_EQ(value.data.type, acpi::AmlData::Type::eQword);
    ASSERT_EQ(std::get<uint64_t>(value.data.data), 0x8877665544332211);
}

TEST(AmlDetailTest, DataRefObjectString) {
    uint8_t data[] = {
        0x0D, 'A', 'B', 'C', 'D'
    };

    std::unique_ptr memory = GetAllocatorMemory();
    acpi::AmlAllocator bitmap {memory.get(), memory.get() + kSize, 16};

    acpi::AmlNodeBuffer code { &bitmap, 2 };
    acpi::AmlParser parser(data);
    acpi::AmlAnyId id = acpi::detail::DataRefObject(parser, code);
    acpi::AmlDataTerm value = *code.get<acpi::AmlDataTerm>(id);

    ASSERT_EQ(value.data.type, acpi::AmlData::Type::eString);
    stdx::StringView view = std::get<stdx::StringView>(value.data.data);
    ASSERT_EQ(view.count(), 4);
    ASSERT_EQ(view[0], 'A');
    ASSERT_EQ(view[1], 'B');
    ASSERT_EQ(view[2], 'C');
    ASSERT_EQ(view[3], 'D');
}

TEST(AmlDetailTest, Revision0Ones) {
    uint8_t data[] = { 0xFF };

    std::unique_ptr memory = GetAllocatorMemory();
    acpi::AmlAllocator bitmap {memory.get(), memory.get() + kSize, 16};

    acpi::AmlNodeBuffer code { &bitmap, 0 };
    acpi::AmlParser parser(data);
    acpi::AmlAnyId id = acpi::detail::DataRefObject(parser, code);
    acpi::AmlDataTerm value = *code.get<acpi::AmlDataTerm>(id);

    ASSERT_EQ(value.data.type, acpi::AmlData::Type::eDword);
    ASSERT_EQ(std::get<uint64_t>(value.data.data), 0xFFFF'FFFF);
}

TEST(AmlDetailTest, Revision2Ones) {
    uint8_t data[] = { 0xFF, 0xFF };

    std::unique_ptr memory = GetAllocatorMemory();
    acpi::AmlAllocator bitmap {memory.get(), memory.get() + kSize, 16};

    acpi::AmlNodeBuffer code { &bitmap, 2 };
    acpi::AmlParser parser(data);
    acpi::AmlAnyId id = acpi::detail::DataRefObject(parser, code);
    acpi::AmlDataTerm value = *code.get<acpi::AmlDataTerm>(id);

    ASSERT_EQ(value.data.type, acpi::AmlData::Type::eQword);
    ASSERT_EQ(std::get<uint64_t>(value.data.data), 0xFFFF'FFFF'FFFF'FFFF);
}

TEST(AmlDetailTest, ZeroData) {
    uint8_t data[] = { 0x00 };

    std::unique_ptr memory = GetAllocatorMemory();
    acpi::AmlAllocator bitmap {memory.get(), memory.get() + kSize, 16};

    acpi::AmlNodeBuffer code { &bitmap, 0 };
    acpi::AmlParser parser(data);
    acpi::AmlAnyId id = acpi::detail::DataRefObject(parser, code);
    acpi::AmlDataTerm value = *code.get<acpi::AmlDataTerm>(id);

    ASSERT_EQ(value.data.type, acpi::AmlData::Type::eByte);
    ASSERT_EQ(std::get<uint64_t>(value.data.data), 0);
}

TEST(AmlDetailTest, OneData) {
    uint8_t data[] = { 0x01 };

    std::unique_ptr memory = GetAllocatorMemory();
    acpi::AmlAllocator bitmap {memory.get(), memory.get() + kSize, 16};

    acpi::AmlNodeBuffer code { &bitmap, 0 };
    acpi::AmlParser parser(data);
    acpi::AmlAnyId id = acpi::detail::DataRefObject(parser, code);
    acpi::AmlDataTerm value = *code.get<acpi::AmlDataTerm>(id);

    ASSERT_EQ(value.data.type, acpi::AmlData::Type::eByte);
    ASSERT_EQ(std::get<uint64_t>(value.data.data), 1);
}

TEST(AmlDetailTest, PkgLength1) {
    const uint8_t data[2] = {
        0x4C, 0x05,
    };

    acpi::AmlParser parser(data);
    uint32_t length = acpi::detail::PkgLength(parser);

    ASSERT_EQ(length, 92);
}

TEST(AmlDetailTest, PkgLength2) {
    const uint8_t data[2] = {
        0x3F, 0x5B,
    };

    acpi::AmlParser parser(data);
    uint32_t length = acpi::detail::PkgLength(parser);

    ASSERT_EQ(length, 63);
}

TEST(AmlDetailTest, IfElseCondRef) {
    const uint8_t data[] = {
        0xA0, 0x3F, 0x5B, 0x12, 0x5C, 0x5F, 0x4F, 0x53, 0x49, 0x00, 0xA0, 0x1A, 0x5F, 0x4F, 0x53, 0x49,
        0x0D, 0x57, 0x69, 0x6E, 0x64, 0x6F, 0x77, 0x73, 0x20, 0x32, 0x30, 0x30, 0x39, 0x00, 0x70, 0x0A,
        0x50, 0x54, 0x53, 0x4F, 0x53, 0xA0, 0x1A, 0x5F, 0x4F, 0x53, 0x49, 0x0D, 0x57, 0x69, 0x6E, 0x64,
        0x6F, 0x77, 0x73, 0x20, 0x32, 0x30, 0x31, 0x35, 0x00, 0x70, 0x0A, 0x70, 0x54, 0x53, 0x4F, 0x53,
        0x10, 0x41,
    };

    std::unique_ptr memory = GetAllocatorMemory();
    acpi::AmlAllocator bitmap {memory.get(), memory.get() + kSize, 16};

    acpi::AmlNodeBuffer code { &bitmap, 0 };
    acpi::AmlParser parser(data);
    acpi::AmlAnyId op = acpi::detail::Term(parser, code);
    acpi::AmlBranchTerm *branch = code.get<acpi::AmlBranchTerm>(op);

    ASSERT_NE(branch, nullptr);

    ASSERT_NE(branch->predicate, acpi::AmlAnyId::kInvalid);
    ASSERT_EQ(code.getType(branch->predicate), acpi::AmlTermType::eCondRefOf);

    acpi::AmlCondRefOfTerm *predicate = code.get<acpi::AmlCondRefOfTerm>(branch->predicate);
    ASSERT_NE(predicate, nullptr);
    ASSERT_EQ(std::get<acpi::AmlName>(predicate->name), "\\_OSI"_aml);
}

TEST(AmlDetailTest, Method) {
    const uint8_t data[] = {
        //  MethodOp  PkgLength '_'   'P'   'I'   'C'   MethodFlags
            0x14,     0x2E,     0x5F, 0x50, 0x49, 0x43, 0x01,
        // Method (_PIC, 1, NotSerialized) {

        //  IfOp  PkgLength Arg0
            0xA0, 0x18,     0x68,
        // If (Arg0) {

        0x70, 0x0A, 0xAA, 0x44, 0x42, 0x47, 0x38,
        // DBG8 = 0xAA
        0x5C, 0x2F, 0x03, 0x5F, 0x53, 0x42, 0x5F, 0x50, 0x43, 0x49, 0x30, 0x4E, 0x41, 0x50, 0x45,
        // \_SB.PCI0.NAPE ()
        0xA1, 0x08,
        // } Else {
        0x70, 0x0A, 0xAC, 0x44, 0x42, 0x47, 0x38,
        // DBG8 = 0xAC
        // }
        0x70, 0x68, 0x50, 0x49, 0x43, 0x4D,
        // PICM = Arg0

        0x08, 0x4F, 0x53, 0x56, 0x52, 0xFF, 0x14, 0x40, 0x2A, 0x4F, 0x53, 0x46, 0x4C,
    };

    std::unique_ptr memory = GetAllocatorMemory();
    acpi::AmlAllocator bitmap {memory.get(), memory.get() + kSize, 16};

    acpi::AmlNodeBuffer code { &bitmap, 0 };
    acpi::AmlParser parser(data);
    acpi::AmlAnyId term = acpi::detail::Term(parser, code);

    ASSERT_FALSE(code.error());

    ASSERT_EQ(code.getType(term), acpi::AmlTermType::eMethod);

    acpi::AmlMethodTerm method = *code.get<acpi::AmlMethodTerm>(term);

    ASSERT_EQ(method.name, "_PIC"_aml);
    ASSERT_EQ(method.terms.count(), 2);

    acpi::AmlBranchTerm *branch = code.get<acpi::AmlBranchTerm>(method.terms[0]);
    ASSERT_NE(branch, nullptr);

    ASSERT_NE(branch->predicate, acpi::AmlAnyId::kInvalid);
    ASSERT_EQ(code.getType(branch->predicate), acpi::AmlTermType::eData);

    acpi::AmlDataTerm *predicate = code.get<acpi::AmlDataTerm>(branch->predicate);
    ASSERT_NE(predicate, nullptr);
    ASSERT_EQ(std::get<acpi::AmlArgObject>(predicate->data.data), acpi::AmlArgObject::eArg0);

    ASSERT_EQ(branch->terms.count(), 2);

    {
        acpi::AmlStoreTerm *store = code.get<acpi::AmlStoreTerm>(branch->terms[0]);
        ASSERT_NE(store, nullptr);

        acpi::AmlDataTerm *source = code.get<acpi::AmlDataTerm>(store->source);
        ASSERT_NE(source, nullptr);
        ASSERT_EQ(std::get<uint64_t>(source->data.data), 0xAA);

        ASSERT_EQ(std::get<acpi::AmlName>(store->target), "DBG8"_aml);
    }

    {
        acpi::AmlMethodInvokeTerm *invoke = code.get<acpi::AmlMethodInvokeTerm>(branch->terms[1]);
        ASSERT_NE(invoke, nullptr);

        ASSERT_EQ(invoke->name, "\\_SB.PCI0.NAPE"_aml);
    }

    {
        acpi::AmlElseTerm *otherwise = code.get<acpi::AmlElseTerm>(branch->otherwise);
        ASSERT_NE(otherwise, nullptr);

        ASSERT_EQ(otherwise->terms.count(), 1);

        acpi::AmlStoreTerm *store = code.get<acpi::AmlStoreTerm>(otherwise->terms[0]);
        ASSERT_NE(store, nullptr);

        acpi::AmlDataTerm *source = code.get<acpi::AmlDataTerm>(store->source);
        ASSERT_NE(source, nullptr);
        ASSERT_EQ(std::get<uint64_t>(source->data.data), 0xAC);

        ASSERT_EQ(std::get<acpi::AmlName>(store->target), "DBG8"_aml);
    }
}
