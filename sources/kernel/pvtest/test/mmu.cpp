#include <barrier>
#include <gtest/gtest.h>

#include "pvtest/cpu.hpp"
#include "pvtest/machine.hpp"

#include "memory/pte.hpp"

#include <sys/mman.h>
#include <thread>

class PvMmuTest : public testing::Test {
public:
    void SetUp() override {
        pv::Machine::init();
        auto shared = pv::Machine::getSharedMemory();
        ASSERT_TRUE(shared.isValid()) << "Shared memory must be valid " << std::string_view(km::format(shared));

        machine = new pv::Machine(4);
        ASSERT_NE(machine, nullptr);
        ASSERT_TRUE(shared.contains((void*)machine)) << "Machine " << (void*)machine << " must be allocated in shared memory " << std::string_view(km::format(shared));
    }

    void TearDown() override {
        delete machine;
        pv::Machine::finalize();
    }

    pv::Machine *machine = nullptr;
};

TEST_F(PvMmuTest, Construct) {
    pv::Memory *memory = machine->getMemory();
    ASSERT_NE(memory, nullptr);
    ASSERT_FALSE(memory->getHostMemory().isNull());
    ASSERT_NE(memory->getMemorySize(), 0);
}

std::barrier gBarrier(2);
static const char kMessage[] = "Functional mmio test passed\n";

void TestEmulateMmio(void *address) {
    memcpy(address, kMessage, sizeof(kMessage));
    gBarrier.arrive_and_wait();

    // sleep forever
    for (;;)
        std::this_thread::sleep_for(std::chrono::seconds(60));
}

TEST_F(PvMmuTest, EmulateMmio) {
    OsStatus status = OsStatusSuccess;
    km::PageTables ptes;
    pv::Memory *memory = machine->getMemory();

    km::AddressMapping hostPteMemory = memory->addSection({
        .type = boot::MemoryRegion::eBootloaderReclaimable,
        .range = { sm::megabytes(1).bytes(), sm::megabytes(2).bytes() },
    });

    km::AddressMapping guestInitMemory = memory->addSection({
        .type = boot::MemoryRegion::eKernel,
        .range = { sm::megabytes(2).bytes(), sm::megabytes(3).bytes() },
    });

    ASSERT_NE(hostPteMemory.vaddr, nullptr) << "Failed to add boot memory section";
    ASSERT_NE(guestInitMemory.vaddr, nullptr) << "Failed to add guest init memory section";

    status = km::PageTables::create(machine->getPageBuilder(), hostPteMemory, km::PageFlags::eAll, &ptes);
    ASSERT_EQ(status, OsStatusSuccess) << "Failed to create page tables";

    void *guest = memory->getGuestMemory();
    km::AddressMapping guestMapping {
        .vaddr = guest,
        .paddr = guestInitMemory.paddr,
        .size = sm::megabytes(1).bytes(),
    };

    status = ptes.map(guestMapping, km::PageFlags::eAll);
    ASSERT_EQ(status, OsStatusSuccess) << "Failed to map guest memory";

    machine->bspInit({
        .gregs = {
            [REG_RDI] = (greg_t)guest,
            [REG_RIP] = (greg_t)TestEmulateMmio,
        }
    });

    gBarrier.arrive_and_wait();

    int eq = memcmp(guest, kMessage, sizeof(kMessage));
    ASSERT_EQ(eq, 0) << "Failed to read mmio message";
}
