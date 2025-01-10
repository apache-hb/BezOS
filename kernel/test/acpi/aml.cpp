#include <gtest/gtest.h>

#include <fstream>
#include <vector>
#include <filesystem>

#include "acpi/aml.hpp"

#include "allocator/bucket.hpp"

using namespace stdx::literals;

static std::vector<uint8_t> ReadBinaryBlob(const std::filesystem::path& path) {
    std::ifstream ifs{path, std::ios::binary};
    EXPECT_TRUE(ifs.is_open()) << "Failed to open file: " << path;

    ifs.seekg(0, std::ios::end);
    size_t size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(size);
    ifs.read(reinterpret_cast<char*>(data.data()), size);

    return data;
}

struct AcpiTable {
    std::vector<uint8_t> blob;
    size_t offset = 0;

    const acpi::RsdtHeader *next() {
        if (offset > blob.size())
            return nullptr;

        acpi::RsdtHeader *header = reinterpret_cast<acpi::RsdtHeader*>(blob.data() + offset);
        offset += header->length;

        return header;
    }
};

TEST(AmlTest, ParseTable) {
    const char *path = getenv("AML_BLOB");
    ASSERT_NE(path, nullptr) << "AML_BLOB environment variable not set";

    AcpiTable table = { ReadBinaryBlob(path) };

    const acpi::RsdtHeader *header = nullptr;
    do {
        header = table.next();
    } while (header != nullptr && header->signature != "DSDT"_sv);

    ASSERT_NE(header, nullptr) << "DSDT table not found";

    static constexpr size_t kBufferSize0 = 0x10000;
    static constexpr size_t kBufferSize1 = 0x10000;
    static constexpr size_t kBufferSize2 = 0x100000;
    std::unique_ptr<uint8_t[]> buffer0{new uint8_t[kBufferSize0]};
    std::unique_ptr<uint8_t[]> buffer1{new uint8_t[kBufferSize1]};
    std::unique_ptr<uint8_t[]> buffer2{new uint8_t[kBufferSize2]};

    mem::BitmapAllocator arena0 { buffer0.get(), buffer0.get() + kBufferSize0 };
    mem::BitmapAllocator arena1 { buffer1.get(), buffer1.get() + kBufferSize1 };
    mem::BitmapAllocator arena2 { buffer2.get(), buffer2.get() + kBufferSize2 };

    mem::BucketAllocator arena { {
        { .width = 32, .allocator = &arena0 },
        { .width = 256, .allocator = &arena1 },
        { .width = 512, .allocator = &arena2 },
    } };

    acpi::WalkAml(header, &arena);
}
