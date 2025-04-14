#include "log.hpp"
#include "system/schedule.hpp"

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

static bool ScheduleInner(km::IsrContext *context, km::IsrContext *newContext) {
    km::IApic *apic = km::GetCpuLocalApic();
    defer { apic->eoi(); };

    // If we find a new thread to schedule then return its context to switch to it
    if (sys2::CpuLocalSchedule *schedule = tlsSchedule.get()) {
        return schedule->scheduleNextContext(context, newContext);
    }

    return false;
}

static km::IsrContext ScheduleInt(km::IsrContext *context) {
    km::IsrContext newContext;
    if (ScheduleInner(context, &newContext)) {
        // KmDebugMessage("[TASK] Work ", km::GetCurrentCoreId(), "\n");
        return newContext;
    }

    // Otherwise we idle until the next interrupt
    // KmDebugMessage("[TASK] Idle ", km::GetCurrentCoreId(), "\n");
    KmIdle();
}

static constexpr std::chrono::milliseconds kDefaultTimeSlice = 5ms;

void sys2::InstallTimerIsr(km::LocalIsrTable *table) {
    km::IsrCallback old = table->install(km::isr::kTimerVector, ScheduleInt);

    if (old != km::DefaultIsrHandler && old != ScheduleInt) {
        KmDebugMessage("Failed to install scheduler isr ", (void*)old, " != ", (void*)km::DefaultIsrHandler, "\n");
        KM_PANIC("Failed to install scheduler isr.");
    }
}

void sys2::EnterScheduler(km::LocalIsrTable *table, CpuLocalSchedule *scheduler, km::ApicTimer *apicTimer) {
    km::IApic *apic = km::GetCpuLocalApic();
    tlsSchedule = scheduler;
    KmSystemCallStackTlsOffset = tlsKernelStack.tlsOffset();
    InstallTimerIsr(table);

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

sm::RcuSharedPtr<sys2::Process> sys2::GetCurrentProcess() {
    if (auto schedule = tlsSchedule.get()) {
        return schedule->currentProcess();
    }

    return nullptr;
}

sm::RcuSharedPtr<sys2::Thread> sys2::GetCurrentThread() {
    if (auto schedule = tlsSchedule.get()) {
        return schedule->currentThread();
    }

    return nullptr;
}
