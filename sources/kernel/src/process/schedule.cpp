#include "process/schedule.hpp"

#include "kernel.hpp"
#include "allocator/synchronized.hpp"
#include "allocator/tlsf.hpp"

CPU_LOCAL
static constinit km::CpuLocal<sm::RcuSharedPtr<km::Thread>> tlsCurrentThread;

CPU_LOCAL
static constinit km::CpuLocal<uint8_t> tlsScheduleIdx;

extern "C" [[noreturn]] void KmResumeThread(km::IsrContext *context);

using SynchronizedTlsfAllocator = mem::SynchronizedAllocator<mem::TlsfAllocator>;

static constinit mem::IAllocator *gSchedulerAllocator = nullptr;

void km::InitSchedulerMemory(void *memory, size_t size) {
    KM_CHECK(memory != nullptr, "Invalid memory for scheduler allocator.");
    KM_CHECK(size > 0, "Invalid size for scheduler allocator.");
    KM_CHECK(gSchedulerAllocator == nullptr, "Scheduler allocator already initialized.");

    gSchedulerAllocator = new SynchronizedTlsfAllocator(memory, size);
}

void *km::SchedulerQueueTraits::malloc(size_t size) {
    return gSchedulerAllocator->allocate(size);
}

void km::SchedulerQueueTraits::free(void *ptr) {
    gSchedulerAllocator->deallocate(ptr, 0);
}

km::Scheduler::Scheduler()
    : mQueue()
{ }

// TODO: remove these interrupt guards when i figure out how to write an allocator
// thats reentrant and doesn't deadlock

void km::Scheduler::addWorkItem(sm::RcuSharedPtr<Thread> thread) {
    if (thread) {
        // IntGuard guard;
        KmDebugMessage("[SCHED] Adding work item: ", thread->name(), "\n");

        bool ok = mQueue.enqueue(thread);
        KM_CHECK(ok, "Failed to add work item to scheduler.");
    }
}

sm::RcuSharedPtr<km::Thread> km::Scheduler::getWorkItem() noexcept {
    // IntGuard guard;

    sm::RcuWeakPtr<Thread> thread;
    while (mQueue.try_dequeue(thread)) {
        if (sm::RcuSharedPtr<km::Thread> ptr = thread.lock()) {
            KmDebugMessage("[SCHED] Found work item: ", ptr->name(), "\n");
            return ptr;
        }
    }

    return nullptr;
}

sm::RcuSharedPtr<km::Thread> km::GetCurrentThread() {
    return tlsCurrentThread.get();
}

sm::RcuSharedPtr<km::Process> km::GetCurrentProcess() {
    if (sm::RcuSharedPtr<km::Thread> thread = GetCurrentThread()) {
        return thread->process.lock();
    }

    return nullptr;
}

void km::SwitchThread(sm::RcuSharedPtr<km::Thread> next) {
    tlsCurrentThread = next;
    kFsBase.store(next->tlsAddress);
    km::IsrContext *state = &next->state;

    //
    // If we're transitioning out of kernel space then we need to swapgs.
    //
    if ((state->cs & 0b11) != 0) {
        __swapgs();
    }

    KmResumeThread(state);
}

void km::ScheduleWork(LocalIsrTable *table, IApic *apic, sm::RcuSharedPtr<km::Thread> initial) {
    tlsCurrentThread = initial;

    const IsrEntry *scheduleInt = table->allocate([](km::IsrContext *ctx) noexcept -> km::IsrContext {
        IApic *apic = km::GetCpuLocalApic();
        apic->eoi();

        Scheduler *scheduler = km::GetScheduler();

        if (sm::RcuSharedPtr<km::Thread> next = scheduler->getWorkItem()) {
            PhysicalAddress cr3 = next->process.lock()->ptes.root();
            SystemMemory *memory = GetSystemMemory();
            memory->getPager().setActiveMap(cr3);

            if (sm::RcuSharedPtr<km::Thread> current = km::GetCurrentThread()) {
                current->state = *ctx;
                current->tlsAddress = kFsBase.load();
                scheduler->addWorkItem(current);
            } else {
                KmDebugMessage("[SCHED] No current thread to switch from - ", km::GetCurrentCoreId(), "\n");
            }

            tlsCurrentThread = next;
            kFsBase.store(next->tlsAddress);
            return next->state;
        } else {
            return *ctx;
        }
    });

    tlsScheduleIdx = table->index(scheduleInt);

    apic->setTimerDivisor(apic::TimerDivide::e64);
    apic->setInitialCount(0x10000);

    apic->cfgIvtTimer(apic::IvtConfig {
        .vector = tlsScheduleIdx.get(),
        .polarity = apic::Polarity::eActiveHigh,
        .trigger = apic::Trigger::eEdge,
        .enabled = true,
        .timer = apic::TimerMode::ePeriodic,
    });

    KmIdle();
}

void km::YieldCurrentThread() {
    IApic *apic = km::GetCpuLocalApic();
    apic->selfIpi(tlsScheduleIdx.get());
}
