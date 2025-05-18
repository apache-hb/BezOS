#include "gdt.h"
#include "log.hpp"

#include "logger/categories.hpp"
#include "system/schedule.hpp"
#include "system/thread.hpp"

#include "panic.hpp"
#include "thread.hpp"
#include "common/util/defer.hpp"

#include "timer/apic_timer.hpp"

#include "allocator/synchronized.hpp"

using namespace std::chrono_literals;

using SynchronizedTlsfAllocator = mem::SynchronizedAllocator<mem::TlsfAllocator>;

extern "C" uint64_t KmSystemCallStackTlsOffset;

extern "C" [[noreturn]] void __x86_64_resume(km::IsrContext *context);

CPU_LOCAL
static constinit km::CpuLocal<sys::CpuLocalSchedule*> tlsSchedule;

CPU_LOCAL
static constinit km::CpuLocal<void*> tlsKernelStack;

static constinit mem::IAllocator *gSchedulerAllocator = nullptr;

void sys::SchedulerQueueTraits::init(void *memory, size_t size) {
    KM_CHECK(memory != nullptr, "Invalid memory for scheduler allocator.");
    KM_CHECK(size > 0, "Invalid size for scheduler allocator.");
    KM_CHECK(gSchedulerAllocator == nullptr, "Scheduler allocator already initialized.");

    gSchedulerAllocator = new SynchronizedTlsfAllocator(memory, size);
}

void *sys::SchedulerQueueTraits::malloc(size_t size) {
    return gSchedulerAllocator->allocate(size);
}

void sys::SchedulerQueueTraits::free(void *ptr) {
    gSchedulerAllocator->deallocate(ptr, 0);
}

static bool ScheduleInner(km::IsrContext *context, km::IsrContext *newContext) noexcept [[clang::reentrant]] {
    km::IApic *apic = km::GetCpuLocalApic();
    defer { apic->eoi(); };

    // If we find a new thread to schedule then return its context to switch to it
    if (sys::CpuLocalSchedule *schedule = tlsSchedule.get()) {
        void *syscallStack = nullptr;
        if (schedule->scheduleNextContext(context, newContext, &syscallStack)) {
            // We need to set the syscall stack pointer to the new thread's stack
            tlsKernelStack = syscallStack;
            return true;
        }
    }

    return false;
}

static km::IsrContext ScheduleInt(km::IsrContext *context) noexcept [[clang::reentrant]] {
    km::IsrContext newContext;
    auto oldThread = tlsSchedule.get()->currentThread();
    if (ScheduleInner(context, &newContext)) {
        return newContext;
    }

    if (oldThread != nullptr) {
        KmDebugMessage("[SCHED] Dropping thread ", oldThread->getName(), " - ", km::GetCurrentCoreId(), "\n");
    }

    // Otherwise we idle until the next interrupt
    KmIdle();
}

static constexpr std::chrono::milliseconds kDefaultTimeSlice = 5ms;

void sys::InstallTimerIsr(km::SharedIsrTable *table) {
    km::IsrCallback old = table->install(km::isr::kTimerVector, ScheduleInt);

    if (old != km::DefaultIsrHandler && old != ScheduleInt) {
        TaskLog.fatalf("Failed to install scheduler isr ", (void*)old, " != ", (void*)km::DefaultIsrHandler);
        KM_PANIC("Failed to install scheduler isr.");
    }
}

void sys::EnterScheduler(CpuLocalSchedule *scheduler, km::ApicTimer *apicTimer) {
    km::IApic *apic = km::GetCpuLocalApic();
    tlsSchedule = scheduler;
    KmSystemCallStackTlsOffset = tlsKernelStack.tlsOffset();

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

sm::RcuSharedPtr<sys::Process> sys::GetCurrentProcess() {
    if (auto schedule = tlsSchedule.get()) {
        return schedule->currentProcess();
    }

    return nullptr;
}

sm::RcuSharedPtr<sys::Thread> sys::GetCurrentThread() {
    if (auto schedule = tlsSchedule.get()) {
        return schedule->currentThread();
    }

    return nullptr;
}

void sys::YieldCurrentThread() {
    // TODO: this entire function is kind of a hack.
    // The scheduler should probably deal with this
    arch::Intrin::LongJumpState jmp;
    if (arch::Intrin::setjmp(&jmp)) {
        return;
    }

    // forge a context from the jmpbuf that can be switched to
    km::IsrContext context = {
        .rax = 1,
        .rbx = jmp.rbx,
        .r12 = jmp.r12,
        .r13 = jmp.r13,
        .r14 = jmp.r14,
        .r15 = jmp.r15,
        .rbp = jmp.rbp,
        .rip = jmp.rip,
        .cs = (GDT_64BIT_CODE * 0x8),
        .rflags = 0x202,
        .rsp = jmp.rsp,
        .ss = (GDT_64BIT_DATA * 0x8),
    };

    arch::IntrinX86_64::cli();

    auto next = ScheduleInt(&context);

    if ((next.cs & 0b11) != 0) {
        __swapgs();
    }

    __x86_64_resume(&next);
}
