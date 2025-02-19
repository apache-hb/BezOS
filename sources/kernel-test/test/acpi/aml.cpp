#include <gtest/gtest.h>

#include <fstream>
#include <vector>
#include <filesystem>

#include "acpi/aml.hpp"

#include "allocator/tlsf.hpp"
#include "log.hpp"

using namespace stdx::literals;

class TestAllocator : public mem::IAllocator {
public:
    void* allocate(size_t size, size_t _) override {
        void *ptr = std::malloc(size);
        return ptr;
    }

    void* reallocate(void* old, size_t _, size_t newSize) override {
        return std::realloc(old, newSize);
    }

    void deallocate(void* ptr, size_t _) override {
        std::free(ptr);
    }
};

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
        printer.println("Name(", term->name, ")");
        break;
    }
    case acpi::AmlTermType::eAlias: {
        acpi::AmlAliasTerm *term = nodes.get<acpi::AmlAliasTerm>(id);
        printer.println("Alias(", term->name, ", ", term->alias, ")");
        break;
    }
    case acpi::AmlTermType::eDevice: {
        acpi::AmlDeviceTerm *term = nodes.get<acpi::AmlDeviceTerm>(id);
        printer.println("Device(", term->name, ") {");
        if (!printer.enter(id)) return false;
        for (size_t i = 0; i < term->terms.count(); i++) {
            if (!PrintAml(printer, nodes, term->terms[i])) return false;
        }
        printer.leave();
        printer.println("}");
        break;
    }
    case acpi::AmlTermType::eMethod: {
        acpi::AmlMethodTerm *term = nodes.get<acpi::AmlMethodTerm>(id);
        stdx::StringView serialized = term->flags.serialized() ? "Serialized"_sv : "NotSerialized"_sv;
        printer.println("Method(", term->name, ", ", term->flags.argCount(), ", ", serialized, ") {");
        if (!printer.enter(id)) return false;
        for (size_t i = 0; i < term->terms.count(); i++) {
            if (!PrintAml(printer, nodes, term->terms[i])) return false;
        }
        printer.leave();
        printer.println("}");
        break;
    }
    case acpi::AmlTermType::eOpRegion: {
        acpi::AmlOpRegionTerm *term = nodes.get<acpi::AmlOpRegionTerm>(id);
        printer.println("OperationRegion(", term->name, ")");
        break;
    }
    case acpi::AmlTermType::eField: {
        acpi::AmlFieldTerm *term = nodes.get<acpi::AmlFieldTerm>(id);
        stdx::StringView lock = term->flags.lock() ? "Lock"_sv : "NoLock"_sv;
        printer.println("Field(", term->name, ", ", lock, ")");
        break;
    }
    case acpi::AmlTermType::eIndexField: {
        acpi::AmlIndexFieldTerm *term = nodes.get<acpi::AmlIndexFieldTerm>(id);
        stdx::StringView lock = term->flags.lock() ? "Lock"_sv : "NoLock"_sv;
        printer.println("IndexField(", term->name, ", ", term->field, ", ", lock, ")");
        break;
    }
    case acpi::AmlTermType::eCreateByteField: {
        acpi::AmlCreateByteFieldTerm *term = nodes.get<acpi::AmlCreateByteFieldTerm>(id);
        printer.println("CreateByteField(", term->name, ")");
        break;
    }
    case acpi::AmlTermType::eCreateWordField: {
        acpi::AmlCreateWordFieldTerm *term = nodes.get<acpi::AmlCreateWordFieldTerm>(id);
        printer.println("CreateWordField(", term->name, ")");
        break;
    }
    case acpi::AmlTermType::eCreateDwordField: {
        acpi::AmlCreateDwordFieldTerm *term = nodes.get<acpi::AmlCreateDwordFieldTerm>(id);
        printer.println("CreateDwordField(", term->name, ")");
        break;
    }
    case acpi::AmlTermType::eCreateQwordField: {
        acpi::AmlCreateQwordFieldTerm *term = nodes.get<acpi::AmlCreateQwordFieldTerm>(id);
        printer.println("CreateQwordField(", term->name, ")");
        break;
    }
    case acpi::AmlTermType::eExternal: {
        acpi::AmlExternalTerm *term = nodes.get<acpi::AmlExternalTerm>(id);
        printer.println("External(", term->name, ", ", term->type, ", ", term->args, ")");
        break;
    }
    case acpi::AmlTermType::eIfElse: {
        acpi::AmlBranchTerm *term = nodes.get<acpi::AmlBranchTerm>(id);
        printer.println("If {");
        if (!printer.enter(id)) return false;
        if (!PrintAml(printer, nodes, term->predicate)) return false;
        for (size_t i = 0; i < term->terms.count(); i++) {
            if (!PrintAml(printer, nodes, term->terms[i])) return false;
        }
        printer.leave();

        if (term->otherwise) {
            printer.println("} Else {");
            if (!printer.enter(id)) return false;
            for (size_t i = 0; i < term->terms.count(); i++) {
                if (!PrintAml(printer, nodes, term->terms[i])) return false;
            }
            printer.leave();
        }

        printer.println("}");
        break;
    }
    case acpi::AmlTermType::eElse: {
        acpi::AmlElseTerm *term = nodes.get<acpi::AmlElseTerm>(id);
        printer.println("Else {");
        if (!printer.enter(id)) return false;
        for (size_t i = 0; i < term->terms.count(); i++) {
            if (!PrintAml(printer, nodes, term->terms[i])) return false;
        }
        printer.leave();
        printer.println("}");
        break;
    }
    case acpi::AmlTermType::eScope: {
        acpi::AmlScopeTerm *term = nodes.get<acpi::AmlScopeTerm>(id);
        printer.println("Scope(", term->name, ") {");
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

    static constexpr size_t kBufferSize = 0x1000000;
    std::unique_ptr<uint8_t[]> buffer{new uint8_t[kBufferSize]};

    mem::TlsfAllocator tlsf { buffer.get(), kBufferSize };

    acpi::AmlCode code = acpi::WalkAml(header, &tlsf);

    acpi::AmlNodeBuffer& nodes = code.nodes();
    acpi::AmlScopeTerm *root = code.root();

#if 0
    acpi::DeviceHandle ps2Keyboard = code.findDevice("PNP0303");
#endif

    AmlPrinter printer;

    KmDebugMessage("DefinitionBlock(\"", std::string_view(path), "\", \"DSDT\") {\n");
    printer.enter(code.rootId());
    for (size_t i = 0; i < root->terms.count(); i++) {
        acpi::AmlAnyId id = root->terms[i];
        if (!PrintAml(printer, nodes, id)) break;
    }
    printer.leave();
    KmDebugMessage("}\n");
}
