#include <gtest/gtest.h>

#include "apic.hpp"

#include "test/ktest.hpp"

struct ApicReg {
    uint32_t value;
    std::byte pad[12];

    constexpr operator uint32_t&() {
        return value;
    }
};

static_assert(sizeof(ApicReg) == 16);

/// @brief All apic state registers
/// Lifted from Intel SDM vol 3 Table 12-1. Local APIC Register Address Map
struct alignas(0x1000) ApicState {
    ApicReg reserved0[2];

    ApicReg id;
    ApicReg version;

    ApicReg reserved1[4];

    ApicReg tpr;
    ApicReg apr;
    ApicReg ppr;
    ApicReg eoi;
    ApicReg rrd;
    ApicReg ldr;
    ApicReg dfr;
    ApicReg svr;

    ApicReg isr[8];
    ApicReg tmr[8];
    ApicReg irr[8];
    ApicReg esr;

    ApicReg reserved2[6];

    ApicReg cmci;

    ApicReg icr[2];

    ApicReg lvtTimer;
    ApicReg lvtThermal;
    ApicReg lvtPerf;
    ApicReg lvtLint0;
    ApicReg lvtLint1;
    ApicReg lvtError;
    ApicReg timerInitial;
    ApicReg timerCurrent;

    ApicReg reserved3[4];

    ApicReg timerDivide;

    ApicReg reserved4[1];
};

static_assert(offsetof(ApicState, id) == 0x20);
static_assert(offsetof(ApicState, tpr) == 0x80);
static_assert(offsetof(ApicState, isr) == 0x100);
static_assert(offsetof(ApicState, tmr) == 0x180);
static_assert(offsetof(ApicState, irr) == 0x200);
static_assert(offsetof(ApicState, esr) == 0x280);
static_assert(offsetof(ApicState, cmci) == 0x2f0);
static_assert(offsetof(ApicState, icr) == 0x300);
static_assert(offsetof(ApicState, lvtTimer) == 0x320);
static_assert(offsetof(ApicState, lvtThermal) == 0x330);
static_assert(offsetof(ApicState, lvtPerf) == 0x340);
static_assert(offsetof(ApicState, lvtLint0) == 0x350);
static_assert(offsetof(ApicState, lvtLint1) == 0x360);
static_assert(offsetof(ApicState, lvtError) == 0x370);
static_assert(offsetof(ApicState, timerInitial) == 0x380);
static_assert(offsetof(ApicState, timerCurrent) == 0x390);
static_assert(offsetof(ApicState, timerDivide) == 0x3e0);

#if 0
static constexpr inline auto kReadOnlyRegisters = std::to_array<uintptr_t>({
    offsetof(ApicState, version),
    offsetof(ApicState, apr),
    offsetof(ApicState, ppr),

    offsetof(ApicState, isr[0]), offsetof(ApicState, isr[1]),
    offsetof(ApicState, isr[2]), offsetof(ApicState, isr[3]),
    offsetof(ApicState, isr[4]), offsetof(ApicState, isr[5]),
    offsetof(ApicState, isr[6]), offsetof(ApicState, isr[7]),

    offsetof(ApicState, tmr[0]), offsetof(ApicState, tmr[1]),
    offsetof(ApicState, tmr[2]), offsetof(ApicState, tmr[3]),
    offsetof(ApicState, tmr[4]), offsetof(ApicState, tmr[5]),
    offsetof(ApicState, tmr[6]), offsetof(ApicState, tmr[7]),

    offsetof(ApicState, irr[0]), offsetof(ApicState, irr[1]),
    offsetof(ApicState, irr[2]), offsetof(ApicState, irr[3]),
    offsetof(ApicState, irr[4]), offsetof(ApicState, irr[5]),
    offsetof(ApicState, irr[6]), offsetof(ApicState, irr[7]),

    offsetof(ApicState, timerCurrent),
});
#endif

class SyntheticApic : public kmtest::IMmio {
    ApicState *mHostState = nullptr;
    void *mGuestAddress = nullptr;

    uintptr_t mLastWrite = 0;

public:
    void init(ApicState *state, void *guest) {
        mHostState = state;
        mGuestAddress = guest;
    }

    void move(void *guest) {
        mGuestAddress = guest;
    }

    void *guestAddress() {
        return mGuestAddress;
    }

    uint64_t icr = 0;

    bool enabled = false;

    void readBegin(uintptr_t offset) override {
        ASSERT_EQ(offset % 0x10, 0) << "Apic registers must be aligned to 16 bytes";

        if (!enabled) {
            dprintf(2, "APIC not enabled\n");
            exit(1);
        }
    }

    void writeBegin(uintptr_t offset) override {
        ASSERT_EQ(offset % 0x10, 0) << "Apic registers must be aligned to 16 bytes";

        if (!enabled) {
            dprintf(2, "APIC not enabled\n");
            exit(1);
        }
    }

    void writeEnd(uintptr_t offset) override {
        switch (offset) {
        case offsetof(ApicState, icr[1]): {
            ASSERT_NE(mLastWrite, offset) << "Duplicate write to ICR1";
            icr = uint64_t(mHostState->icr[1]);
            break;
        }
        case offsetof(ApicState, icr[0]): {
            ASSERT_EQ(mLastWrite, offsetof(ApicState, icr[1])) << "ICR0 must be written after ICR1";
            icr |= uint64_t(mHostState->icr[0]) << 32;
            break;
        }
        }

        mLastWrite = offset;
    }
};

class ApicMsr : public kmtest::IMsrDevice {
    uint64_t mApicMsr;
    SyntheticApic *mApic;

public:
    void init(SyntheticApic *apic) {
        mApic = apic;
        mApicMsr = (uintptr_t)mApic | (1 << 8) | (1 << 11);
        mApic->enabled = true;
    }

    uint64_t rdmsr(uint32_t msr) override {
        if (msr != 0x1B) {
            dprintf(2, "unknown msr %u\n", msr);
            exit(1);
        }

        return mApicMsr;
    }

    void wrmsr(uint32_t msr, uint64_t value) override {
        if (msr != 0x1B) {
            dprintf(2, "unknown msr %u\n", msr);
            exit(1);
        }

        // BSP bit is read-only
        uint64_t change = value ^ mApicMsr;
        if (change & (1 << 8)) {
            dprintf(2, "Wrote to APIC BSP bit\n");
            exit(1);
        }

        mApic->enabled = value & 1;
        mApicMsr = value;
    }
};

class ApicTest : public ::testing::Test {
public:
    static void SetUpTestSuite() {
        kmtest::Machine::setup();
    }

    static void TearDownTestSuite() {
        kmtest::Machine::finalize();
    }

    void SetUp() override {
        kmtest::Machine *machine = new kmtest::Machine();

        void *guest = (void*)0xFEE0'0000;
        auto *apic = machine->createMmioObject<ApicState>();
        machine->addMmioRegion(&mApic, apic, guest, sizeof(ApicState));

        mApic.init(apic, guest);

        mMsr.init(&mApic);

        machine->msr(0x1B, &mMsr);

        machine->reset(machine);
    }

    SyntheticApic mApic;
    ApicMsr mMsr;
};

TEST_F(ApicTest, SendIpi) {
    km::LocalApic lapic { km::TlsfAllocation{}, mApic.guestAddress() };
    lapic.selfIpi(0x20);

    ASSERT_EQ(mApic.icr, 0x0004'4020'0000'0000);
}
