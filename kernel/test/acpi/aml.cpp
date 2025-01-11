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

struct AmlPrinter {
    std::vector<acpi::AmlAnyId> stack;

    int depth = 0;

    void println(auto&&... args) {
        for (int i = 0; i < depth; i++) {
            KmDebugMessage("  ");
        }

        KmDebugMessage(args...);

        KmDebugMessage("\n");
    }

    bool enter(acpi::AmlAnyId id) {
        depth++;

        if (std::find(stack.begin(), stack.end(), id) != stack.end()) {
            KmDebugMessage("Cycle detected: ", id.id, "\n");
            return false;
        }

        stack.push_back(id);

        return true;
    }

    void leave() {
        depth--;
        stack.pop_back();
    }
};

static bool PrintAml(AmlPrinter& printer, acpi::AmlNodeBuffer& nodes, acpi::AmlAnyId id) {
    acpi::AmlTermType type = nodes.getType(id);
    switch (type) {
    case acpi::AmlTermType::eName: {
        acpi::AmlNameTerm *term = nodes.get<acpi::AmlNameTerm>(id);
        printer.println("Name(", term->name, ") - ", id.id);
        break;
    }
    case acpi::AmlTermType::eAlias: {
        acpi::AmlAliasTerm *term = nodes.get<acpi::AmlAliasTerm>(id);
        printer.println("Alias(", term->name, ", ", term->alias, ") - ", id.id);
        break;
    }
    case acpi::AmlTermType::eDevice: {
        acpi::AmlDeviceTerm *term = nodes.get<acpi::AmlDeviceTerm>(id);
        printer.println("Device(", term->name, ") - ", id.id);
        break;
    }
    case acpi::AmlTermType::eMethod: {
        acpi::AmlMethodTerm *term = nodes.get<acpi::AmlMethodTerm>(id);
        stdx::StringView serialized = term->flags.serialized() ? "Serialized"_sv : "NotSerialized"_sv;
        printer.println("Method(", term->name, ", ", term->flags.argCount(), ", ", serialized, ") - ", id.id);
        break;
    }
    case acpi::AmlTermType::eScope: {
        acpi::AmlScopeTerm *term = nodes.get<acpi::AmlScopeTerm>(id);
        printer.println("Scope(", term->name, ") - ", id.id, " - ", (void*)term->terms.data(), " {");
        if (!printer.enter(id)) return false;

        for (size_t i = 0; i < term->terms.count(); i++) {
            if (!PrintAml(printer, nodes, term->terms[i])) return false;
        }
        printer.leave();
        printer.println("}");
        break;
    }
    default: {
        printer.println("Unknown(", type, ") - ", id.id);
        break;
    }
    }

    return true;
}

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

    AmlPrinter printer;

    KmDebugMessage("Scope(", root->name, ") {\n");
    printer.enter(code.rootId());
    for (size_t i = 0; i < root->terms.count(); i++) {
        acpi::AmlAnyId id = root->terms[i];
        if (!PrintAml(printer, nodes, id)) break;
    }
    printer.leave();
    KmDebugMessage("}\n");
}
