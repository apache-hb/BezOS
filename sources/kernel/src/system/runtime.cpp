#include "system/schedule.hpp"
#include "system/thread.hpp"

#include "panic.hpp"
#include "thread.hpp"
#include "util/defer.hpp"

#include "timer/apic_timer.hpp"

using namespace std::chrono_literals;

extern "C" uint64_t KmSystemCallStackTlsOffset;

CPU_LOCAL
static constinit km::CpuLocal<sys2::CpuLocalSchedule*> tlsSchedule;

CPU_LOCAL
static constinit km::CpuLocal<void*> tlsKernelStack;

static km::IsrContext ScheduleInt(km::IsrContext *context) {
    km::IApic *apic = km::GetCpuLocalApic();
    defer { apic->eoi(); };

    if (sys2::CpuLocalSchedule *schedule = tlsSchedule.get()) {
        auto newContext = schedule->serviceSchedulerInt(context);

        if (auto thread = schedule->currentThread()) {
            auto kernelStack = thread->getKernelStack();
            tlsKernelStack = kernelStack.baseAddress();
        }

        return newContext;
    }

    return *context;
}

static constexpr std::chrono::milliseconds kDefaultTimeSlice = 5ms;

void sys2::EnterScheduler(km::LocalIsrTable *table, CpuLocalSchedule *scheduler, km::ApicTimer *apicTimer) {
    km::IApic *apic = km::GetCpuLocalApic();
    tlsSchedule = scheduler;
    KmSystemCallStackTlsOffset = tlsKernelStack.tlsOffset();
    table->install(km::isr::kTimerVector, ScheduleInt);

    auto frequency = apicTimer->frequency();
    auto ticks = (frequency * kDefaultTimeSlice.count()) / 1000;
    apic->setTimerDivisor(km::apic::TimerDivide::e1);
    apic->setInitialCount(uint64_t(ticks / si::hertz));

    apic->cfgIvtTimer(km::apic::IvtConfig {
        .vector = km::isr::kTimerVector,
        .polarity = km::apic::Polarity::eActiveHigh,
        .trigger = km::apic::Trigger::eEdge,
        .enabled = true,
        .timer = km::apic::TimerMode::ePeriodic,
    });

    KmIdle();
}
