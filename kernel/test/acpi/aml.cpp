#include <gtest/gtest.h>

#include <fstream>
#include <vector>
#include <filesystem>

#include "acpi/aml.hpp"

static std::vector<uint8_t> ReadBinaryBlob(const std::filesystem::path& path) {
    std::ifstream ifs{path, std::ios::binary};
    EXPECT_TRUE(ifs.is_open());

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

TEST(AmlTest, Simple) {
    const char *path = getenv("AML_BLOB");
    ASSERT_NE(path, nullptr);
}
