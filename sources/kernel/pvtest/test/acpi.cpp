#include "acpi/acpi.hpp"
#include <gtest/gtest.h>

#include "logger/categories.hpp"
#include "memory/address_space.hpp"
#include "pvtest/cpu.hpp"
#include "pvtest/machine.hpp"

#include "logger/queue.hpp"

#include <sys/mman.h>
#include <thread>

class ConsoleAppender : public km::ILogAppender {
    void write(const km::LogMessageView& message) [[clang::nonreentrant]] override {
        auto name = message.logger->getName();
        auto text = message.message;
        fprintf(stderr, "[%.*s] %.*s\n", (int)name.count(), name.data(),
                (int)text.count(), text.data());
    }
};

static ConsoleAppender consoleAppender;

class AcpiTest : public testing::Test {
public:
    static void SetUpTestSuite() {
        pv::Machine::init();
        km::LogQueue::initGlobalLogQueue(256);
        km::LogQueue::addGlobalAppender(&consoleAppender);
    }

    static void TearDownTestSuite() {
        pv::Machine::finalize();
    }

    void SetUp() override {
        machine = new pv::Machine(4);
        ASSERT_NE(machine, nullptr);

        pv::Memory *memory = machine->getMemory();

        guestInitMemory = memory->addSection({
            .type = boot::MemoryRegion::eKernel,
            .range = { sm::megabytes(1).bytes(), sm::megabytes(2).bytes() },
        });

        guestMemory = memory->addSection({
            .type = boot::MemoryRegion::eKernel,
            .range = { sm::megabytes(4).bytes(), sm::megabytes(16).bytes() },
        });

        acpiTableMemory = memory->addSection({
            .type = boot::MemoryRegion::eAcpiReclaimable,
            .range = { sm::megabytes(64).bytes(), sm::megabytes(128).bytes() },
        });

        ASSERT_NE(guestInitMemory.vaddr, nullptr) << "Failed to add guest init memory section";
        ASSERT_NE(guestMemory.vaddr, nullptr) << "Failed to add page table memory section";
        ASSERT_NE(acpiTableMemory.vaddr, nullptr) << "Failed to add ACPI table memory section";

        pteHostMemory = (void*)new (pv::sharedObjectAlignedAlloc(alignof(x64::page), sizeof(x64::page) * 256)) x64::page[256];
        pteHostMapping = {
            .vaddr = pteHostMemory,
            .paddr = pteHostMemory.address,
            .size = sizeof(x64::page) * 256,
        };

        ASSERT_NE(memory->getGuestMemory(), 0) << "Failed to get guest memory";
    }

    void TearDown() override {
        delete machine;
    }

    sm::VirtualAddress pteHostMemory;
    km::VirtualRangeEx testAddressSpace;

    km::AddressMapping guestInitMemory;
    km::AddressMapping guestMemory;
    km::AddressMapping acpiTableMemory;
    km::AddressMapping pteHostMapping;

    pv::Machine *machine = nullptr;
};

struct Context {
    km::PhysicalAddressEx address;
    std::atomic<bool> barrier;
    pv::Machine *machine;
    km::AddressMapping pteMapping;
    sm::VirtualAddress kernelMemoryRange;
};

void TestEmulateMmio(Context *context) {
    OsStatus status = OsStatusSuccess;
    pv::Machine *machine = context->machine;
    km::AddressMapping pteHostMapping = context->pteMapping;

    km::AddressSpace addressSpace;
    acpi::AcpiTables tables;

    status = km::AddressSpace::create(machine->getPageBuilder(), pteHostMapping, km::PageFlags::eAll, km::VirtualRangeEx::of(context->kernelMemoryRange, sizeof(x64::page) * 256), &addressSpace);
    ASSERT_EQ(status, OsStatusSuccess) << "Failed to create address space";

    x64::Cr3 cr3 = x64::Cr3::of(0);
    cr3.setAddress(addressSpace.root().address);

    machine->getCore(0)->setCr3(cr3);

    status = acpi::AcpiTables::setup({
        .rsdpBaseAddress = context->address,
        .ignoreInvalidRsdpChecksum = false,
    }, addressSpace, &tables);
    ASSERT_EQ(status, OsStatusSuccess) << "Failed to setup ACPI";

    context->barrier.store(true);

    // sleep forever
    for (;;)
        std::this_thread::sleep_for(std::chrono::seconds(60));
}

TEST_F(AcpiTest, AcpiInit) {
    x64::page *stack = (x64::page*)pv::sharedObjectAlignedAlloc(alignof(x64::page), sizeof(x64::page) * 2);
    char *base = (char*)(stack + 2);
    *(uint64_t*)base = 0;

    void *locatorAddress = (void*)((uintptr_t)acpiTableMemory.vaddr);
    void *xsdtAddress = (void*)((uintptr_t)acpiTableMemory.vaddr + 1000);

    acpi::RsdpLocator locator {
        .signature = acpi::RsdpLocator::kSignature,
        .checksum = 0,
        .oemid = { 'P', 'V', 'T', 'E', 'S', 'T' },
        .revision = 2,
        .rsdtAddress = static_cast<uint32_t>((uintptr_t)xsdtAddress),
        .length = 0x20,
        .xsdtAddress = static_cast<uint64_t>((uintptr_t)xsdtAddress),
        .extendedChecksum = 0,
    };
    memcpy(locatorAddress, &locator, sizeof(locator));

    InitLog.infof("RSDP locator: ", sm::VirtualAddress(locatorAddress));
    ASSERT_EQ(locator.signature, acpi::RsdpLocator::kSignature) << "Broken locator RSDP signature";
    ASSERT_EQ(((acpi::RsdpLocator*)locatorAddress)->signature, acpi::RsdpLocator::kSignature) << "Invalid RSDP signature";

    acpi::Xsdt xsdt {
        .header = {
            .signature = acpi::Xsdt::kSignature,
            .length = sizeof(acpi::Xsdt),
            .revision = 1,
            .checksum = 0,
            .oemid = { 'P', 'V', 'T', 'E', 'S', 'T' },
            .tableId = { 'P', 'V', 'T', 'E', 'S', 'T', ' ', ' ' },
            .oemRevision = 1,
            .creatorId = 0x20202020,
            .creatorRevision = 1,
        }
    };
    memcpy(xsdtAddress, &xsdt, sizeof(xsdt));

    InitLog.infof("XSDT: ", sm::VirtualAddress(xsdtAddress));
    ASSERT_EQ(xsdt.header.signature, acpi::Xsdt::kSignature) << "Broken XSDT signature";
    ASSERT_EQ(((acpi::Xsdt*)xsdtAddress)->header.signature, acpi::Xsdt::kSignature) << "Invalid XSDT signature";

    InitLog.infof("Address: ", acpiTableMemory.paddr);
    Context *context = new (pv::sharedObjectMalloc(sizeof(Context))) Context {
        .address = acpiTableMemory.paddr.address,
        .machine = machine,
        .pteMapping = pteHostMapping,
        .kernelMemoryRange = pteHostMemory,
    };

    machine->bspInit({
        .gregs = {
            [REG_RDI] = (greg_t)context,
            [REG_RSP] = (greg_t)(base - 0x8),
            [REG_RBP] = (greg_t)(base - 0x8),
            [REG_RIP] = (greg_t)TestEmulateMmio,
        }
    });

    while (!context->barrier.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    pv::sharedObjectFree(context);
}
