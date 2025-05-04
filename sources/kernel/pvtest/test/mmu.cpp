#include <barrier>
#include <gtest/gtest.h>

#include "log.hpp"
#include "pvtest/cpu.hpp"
#include "pvtest/machine.hpp"

#include "memory/pte.hpp"
#include "pvtest/pvtest.hpp"

#include <sys/mman.h>
#include <thread>

class PvMmuTest : public testing::Test {
public:
    static void SetUpTestSuite() {
        pv::Machine::init();
    }

    static void TearDownTestSuite() {
        pv::Machine::finalize();
    }

    void SetUp() override {
        machine = new pv::Machine(4);
        ASSERT_NE(machine, nullptr);
    }

    void TearDown() override {
        delete machine;
    }

    pv::Machine *machine = nullptr;
};

TEST_F(PvMmuTest, Construct) {
    pv::Memory *memory = machine->getMemory();
    ASSERT_NE(memory, nullptr);
    ASSERT_FALSE(memory->getHostMemory().isNull());
    ASSERT_NE(memory->getMemorySize(), 0);
}

static const char kMessage[] = "Functional mmio test passed\n";

void TestEmulateMmio(void *address, std::atomic<bool> *barrier) {
    fprintf(stderr, "TestEmulateMmio: %p %p %p\n", (void*)address, (void*)barrier, kMessage);
    memcpy(address, kMessage, sizeof(kMessage));
    *barrier = true;

    printf("mmio complete: %p %p %p\n", (void*)address, (void*)barrier, kMessage);

    // sleep forever
    for (;;)
        std::this_thread::sleep_for(std::chrono::seconds(60));
}

TEST_F(PvMmuTest, EmulateMmio) {
    OsStatus status = OsStatusSuccess;
    km::PageTables ptes;
    pv::Memory *memory = machine->getMemory();

    km::AddressMapping guestInitMemory = memory->addSection({
        .type = boot::MemoryRegion::eKernel,
        .range = { sm::megabytes(1).bytes(), sm::megabytes(2).bytes() },
    });

    fprintf(stderr, "[TEST] Guest init memory: %s\n", std::string(std::string_view(km::format(guestInitMemory))).c_str());
    fprintf(stderr, "[TEST] Host memory: %s\n", std::string(std::string_view(km::format(memory->getHostMemory()))).c_str());
    fprintf(stderr, "[TEST] Guest memory: %s\n", std::string(std::string_view(km::format(memory->getGuestMemory()))).c_str());

    ASSERT_NE(guestInitMemory.vaddr, nullptr) << "Failed to add guest init memory section";

    status = km::PageTables::create(machine->getPageBuilder(), guestInitMemory, km::PageFlags::eAll, &ptes);
    ASSERT_EQ(status, OsStatusSuccess) << "Failed to create page tables";

    std::atomic<bool> *barrier = new (pv::SharedObjectMalloc(sizeof(std::atomic<bool>))) std::atomic<bool>(false);

    void *guest = memory->getGuestMemory();
    km::AddressMapping guestMapping {
        .vaddr = guest,
        .paddr = guestInitMemory.paddr,
        .size = sm::megabytes(1).bytes(),
    };

    status = ptes.map(guestMapping, km::PageFlags::eAll);
    ASSERT_EQ(status, OsStatusSuccess) << "Failed to map guest memory";

    x64::Cr3 cr3 = x64::Cr3::of(0);
    cr3.setAddress(ptes.root().address);

    machine->getCore(0)->setCr3(cr3);

    x64::page *stack = (x64::page*)pv::SharedObjectAlignedAlloc(alignof(x64::page), sizeof(x64::page) * 2);
    char *base = (char*)(stack + 2);
    *(uint64_t*)base = 0;
    machine->bspInit({
        .gregs = {
            [REG_RDI] = (greg_t)guest,
            [REG_RSI] = (greg_t)barrier,
            [REG_RSP] = (greg_t)(base - 0x8),
            [REG_RBP] = (greg_t)(base - 0x8),
            [REG_RIP] = (greg_t)TestEmulateMmio,
        }
    });

    while (!barrier->load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    int eq = memcmp(guestInitMemory.vaddr, kMessage, sizeof(kMessage));
    ASSERT_EQ(eq, 0) << "Failed to read mmio message";

    pv::SharedObjectFree(barrier);
}
