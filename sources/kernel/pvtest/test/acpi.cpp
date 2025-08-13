#include "acpi/acpi.hpp"
#include <gtest/gtest.h>

#include "memory/address_space.hpp"
#include "pvtest/cpu.hpp"
#include "pvtest/machine.hpp"

#include "logger/queue.hpp"

#include <sys/mman.h>
#include <thread>

class AcpiTest : public testing::Test {
public:
    static void SetUpTestSuite() {
        pv::Machine::init();
        km::LogQueue::initGlobalLogQueue(256);
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

        pteHostMemory = new x64::page[256];
        km::AddressMapping pteHostMapping = {
            .vaddr = pteHostMemory,
            .paddr = (uintptr_t)pteHostMemory,
            .size = sizeof(x64::page) * 256,
        };

        ASSERT_NE(memory->getGuestMemory(), 0) << "Failed to get guest memory";

        addressSpace = new (pv::sharedObjectMalloc(sizeof(km::AddressSpace))) km::AddressSpace();

        OsStatus status = km::AddressSpace::create(machine->getPageBuilder(), pteHostMapping, km::PageFlags::eAll, memory->getGuestRange(), addressSpace);
        ASSERT_EQ(status, OsStatusSuccess) << "Failed to create address space";

        x64::Cr3 cr3 = x64::Cr3::of(0);
        cr3.setAddress(addressSpace->root().address);

        machine->getCore(0)->setCr3(cr3);
    }

    void TearDown() override {
        std::destroy_at(addressSpace);
        pv::sharedObjectFree(addressSpace);

        delete machine;
        delete pteHostMemory;
    }

    x64::page *pteHostMemory;
    km::VirtualRangeEx testAddressSpace;

    km::AddressMapping guestInitMemory;
    km::AddressMapping guestMemory;
    km::AddressMapping acpiTableMemory;

    pv::Machine *machine = nullptr;
    km::AddressSpace *addressSpace;
};

struct Context {
    km::AddressSpace *addressSpace;
    km::AddressMapping acpiTableMapping;
    km::PhysicalAddressEx address;
    std::atomic<bool> barrier;
};

void TestEmulateMmio(Context *context) {
    acpi::AcpiTables tables;
    OsStatus status = acpi::AcpiTables::setup({
        .rsdpBaseAddress = context->address,
        .ignoreInvalidRsdpChecksum = false,
    }, *context->addressSpace, &tables);
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

    new ((void*)((uintptr_t)acpiTableMemory.vaddr)) acpi::RsdpLocator {
        .signature = acpi::RsdpLocator::kSignature,
        .checksum = 0,
        .oemid = { 'P', 'V', 'T', 'E', 'S', 'T' },
        .revision = 2,
        .rsdtAddress = static_cast<uint32_t>((acpiTableMemory.paddr + 0x1000).address),
        .length = 0x20,
        .xsdtAddress = static_cast<uint64_t>(acpiTableMemory.paddr.address + 0x1000),
        .extendedChecksum = 0,
    };

    new ((void*)((uintptr_t)acpiTableMemory.vaddr + 0x1000)) acpi::Xsdt {
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

    Context *context = new (pv::sharedObjectMalloc(sizeof(Context))) Context {
        .addressSpace = addressSpace,
        .acpiTableMapping = acpiTableMemory,
        .address = acpiTableMemory.paddr.address,
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
