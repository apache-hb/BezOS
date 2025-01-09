#include <gtest/gtest.h>

#include <fstream>
#include <vector>
#include <filesystem>

#include "acpi/aml.hpp"

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

    static constexpr size_t kBufferSize = 0x4000;
    std::unique_ptr<uint8_t[]> buffer{new uint8_t[kBufferSize]};

    acpi::AmlAllocator arena { buffer.get(), buffer.get() + kBufferSize };

    acpi::WalkAml(header, arena);
}
