#include <gtest/gtest.h>

#include <fstream>
#include <vector>
#include <filesystem>

#include "acpi/aml.hpp"

#include "aml_test_common.hpp"

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

    AmlTestContext ctx;

    acpi::AmlCode code = acpi::WalkAml(header, &ctx.arena);

    acpi::AmlNodeBuffer& nodes = code.nodes();
    acpi::AmlScopeTerm *root = code.root();

    KmDebugMessage("Scope(", root->name, ") {\n");
    for (size_t i = 0; i < root->terms.count(); i++) {
        acpi::AmlAnyId id = root->terms[i];
        auto type = nodes.getType(id);
        KmDebugMessage("  Term ", i, ": ", uint32_t(id.id), " ", type, "\n");
        switch (type) {
        case acpi::AmlTermType::eName: {
            acpi::AmlNameTerm *term = nodes.get<acpi::AmlNameTerm>(id);
            KmDebugMessage("  Address: ", (void*)term, "\n");
            KmDebugMessage("  Name(", term->name, ")\n");
            break;
        }
        case acpi::AmlTermType::eAlias: {
            acpi::AmlAliasTerm *term = nodes.get<acpi::AmlAliasTerm>(id);
            KmDebugMessage("  Alias(", term->name, ", ", term->alias, ")\n");
            break;
        }
        case acpi::AmlTermType::eDevice: {
            acpi::AmlDeviceTerm *term = nodes.get<acpi::AmlDeviceTerm>(id);
            KmDebugMessage("  Device(", term->name, ")\n");
            break;
        }

        default: {
            KmDebugMessage("  Unknown term\n");
            break;
        }
        }
    }
    KmDebugMessage("}\n");
}
