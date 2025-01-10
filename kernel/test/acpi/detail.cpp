#include <gtest/gtest.h>

#include "acpi/aml.hpp"

using namespace stdx::literals;

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
    acpi::AmlData value = acpi::detail::DataRefObject(parser, code);

    ASSERT_EQ(value.type, acpi::AmlData::Type::eByte);
    ASSERT_EQ(std::get<uint64_t>(value.data), 0x42);
}

TEST(AmlDetailTest, DataRefObjectWord) {
    uint8_t data[] = { 0x0B, 0xFF, 0x00 };

    std::unique_ptr memory = GetAllocatorMemory();
    acpi::AmlAllocator bitmap {memory.get(), memory.get() + kSize, 16};

    acpi::AmlNodeBuffer code { &bitmap, 2 };
    acpi::AmlParser parser(data);
    acpi::AmlData value = acpi::detail::DataRefObject(parser, code);

    ASSERT_EQ(value.type, acpi::AmlData::Type::eWord);
    ASSERT_EQ(std::get<uint64_t>(value.data), 0x00FF);
}

TEST(AmlDetailTest, DataRefObjectDword) {
    uint8_t data[] = { 0x0C, 0x11, 0x22, 0x33, 0x44 };

    std::unique_ptr memory = GetAllocatorMemory();
    acpi::AmlAllocator bitmap {memory.get(), memory.get() + kSize, 16};

    acpi::AmlNodeBuffer code { &bitmap, 2 };
    acpi::AmlParser parser(data);
    acpi::AmlData value = acpi::detail::DataRefObject(parser, code);

    ASSERT_EQ(value.type, acpi::AmlData::Type::eDword);
    ASSERT_EQ(std::get<uint64_t>(value.data), 0x44332211);
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
    acpi::AmlData value = acpi::detail::DataRefObject(parser, code);

    ASSERT_EQ(value.type, acpi::AmlData::Type::eQword);
    ASSERT_EQ(std::get<uint64_t>(value.data), 0x8877665544332211);
}

TEST(AmlDetailTest, DataRefObjectString) {
    uint8_t data[] = {
        0x0D, 0x04, 'A', 'B', 'C', 'D'
    };

    std::unique_ptr memory = GetAllocatorMemory();
    acpi::AmlAllocator bitmap {memory.get(), memory.get() + kSize, 16};

    acpi::AmlNodeBuffer code { &bitmap, 2 };
    acpi::AmlParser parser(data);
    acpi::AmlData value = acpi::detail::DataRefObject(parser, code);

    ASSERT_EQ(value.type, acpi::AmlData::Type::eString);
    stdx::StringView view = std::get<stdx::StringView>(value.data);
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
    acpi::AmlData value = acpi::detail::DataRefObject(parser, code);

    ASSERT_EQ(value.type, acpi::AmlData::Type::eDword);
    ASSERT_EQ(std::get<uint64_t>(value.data), 0xFFFF'FFFF);
}

TEST(AmlDetailTest, Revision2Ones) {
    uint8_t data[] = { 0xFF, 0xFF };

    std::unique_ptr memory = GetAllocatorMemory();
    acpi::AmlAllocator bitmap {memory.get(), memory.get() + kSize, 16};

    acpi::AmlNodeBuffer code { &bitmap, 2 };
    acpi::AmlParser parser(data);
    acpi::AmlData value = acpi::detail::DataRefObject(parser, code);

    ASSERT_EQ(value.type, acpi::AmlData::Type::eQword);
    ASSERT_EQ(std::get<uint64_t>(value.data), 0xFFFF'FFFF'FFFF'FFFF);
}

TEST(AmlDetailTest, ZeroData) {
    uint8_t data[] = { 0x00 };

    std::unique_ptr memory = GetAllocatorMemory();
    acpi::AmlAllocator bitmap {memory.get(), memory.get() + kSize, 16};

    acpi::AmlNodeBuffer code { &bitmap, 0 };
    acpi::AmlParser parser(data);
    acpi::AmlData value = acpi::detail::DataRefObject(parser, code);

    ASSERT_EQ(value.type, acpi::AmlData::Type::eByte);
    ASSERT_EQ(std::get<uint64_t>(value.data), 0);
}

TEST(AmlDetailTest, OneData) {
    uint8_t data[] = { 0x01 };

    std::unique_ptr memory = GetAllocatorMemory();
    acpi::AmlAllocator bitmap {memory.get(), memory.get() + kSize, 16};

    acpi::AmlNodeBuffer code { &bitmap, 0 };
    acpi::AmlParser parser(data);
    acpi::AmlData value = acpi::detail::DataRefObject(parser, code);

    ASSERT_EQ(value.type, acpi::AmlData::Type::eByte);
    ASSERT_EQ(std::get<uint64_t>(value.data), 1);
}
