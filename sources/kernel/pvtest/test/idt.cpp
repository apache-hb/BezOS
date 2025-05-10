#include <gtest/gtest.h>
#include <thread>

#include "arch/isr.hpp"
#include "isr/isr.hpp"
#include "pvtest/machine.hpp"

extern "C" const char PvIsrTable[];

struct alignas(16) PvIdt {
    static constexpr size_t kCount = 256;
    x64::IdtEntry entries[kCount];
};

static constexpr uint64_t kInterruptMagic = 0xabcd;

extern "C" km::IsrContext PvIsrDispatchRoutine(km::IsrContext *context) {
    km::IsrContext result = *context;
    result.rax = kInterruptMagic;
    result.rbx = context->vector;
    result.rip += 2; // 0f 0b
    return result;
}

class PvIdtTest : public testing::Test {
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

TEST_F(PvIdtTest, EmulateLidt) {
    std::atomic<bool> *result = new (pv::SharedObjectMalloc(sizeof(std::atomic<bool>))) std::atomic<bool>(false);
    std::atomic<bool> *barrier = new (pv::SharedObjectMalloc(sizeof(std::atomic<bool>))) std::atomic<bool>(false);

    machine->bspLaunch([&] {
        arch::IDTR idtr {
            .limit = sizeof(PvIdt) - 1,
            .base = (uintptr_t)PvIsrTable,
        };

        arch::IntrinX86_64::lidt(idtr);

        uint64_t rbx, rax;
        asm volatile(
            "mov $0x0, %%rax\n"
            "mov $0x0, %%rbx\n"
            "ud2\n"
            "mov %%rbx, %0\n"
            "mov %%rax, %1\n"
            : "=r"(rbx), "=r"(rax)
        );

        result->store(rbx == km::isr::UD && rax == kInterruptMagic);
        barrier->store(true);
    });

    while (!barrier->load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(result->load()) << "Lidt test failed";
}
