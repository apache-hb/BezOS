#include "process/schedule.hpp"

#include "isr/isr.hpp"
#include "kernel.hpp"
#include "allocator/synchronized.hpp"
#include "allocator/tlsf.hpp"
#include "debug/debug.hpp"

using SynchronizedTlsfAllocator = mem::SynchronizedAllocator<mem::TlsfAllocator>;

extern "C" [[noreturn]] void KmResumeThread(km::IsrContext *context);

extern "C" uint64_t KmSystemCallStackTlsOffset;

CPU_LOCAL
static constinit km::CpuLocal<km::Thread*> tlsCurrentThread;

CPU_LOCAL
static constinit km::CpuLocal<void*> tlsSystemCallStack;

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

void km::Scheduler::addWorkItem(Thread * thread) {
    if (thread) {
        bool ok = mQueue.enqueue(thread);
        KM_CHECK(ok, "Failed to add work item to scheduler.");
    }
}

km::Thread *km::Scheduler::getWorkItem() noexcept {
    Thread *thread;
    while (mQueue.try_dequeue(thread)) {
        return thread;
    }

    return nullptr;
}

km::Thread *km::GetCurrentThread() {
    return tlsCurrentThread.get();
}

km::Process *km::GetCurrentProcess() {
    if (km::Thread *thread = GetCurrentThread()) {
        return thread->process;
    }

    return nullptr;
}

static void SetCurrentThread(km::Thread *thread) {
    tlsCurrentThread = thread;

    if (thread != nullptr) {
        km::kFsBase.store(thread->tlsAddress);
        tlsSystemCallStack = thread->getSyscallStack();
    }
}

void km::SwitchThread(km::Thread *next) {
    SetCurrentThread(next);
    km::IsrContext *state = &next->state;

    //
    // If we're transitioning out of kernel space then we need to swapgs.
    //
    if ((state->cs & 0b11) != 0) {
        __swapgs();
    }

    KmResumeThread(state);
}

static km::IsrContext SchedulerIsr(km::IsrContext *ctx) noexcept {
    km::IApic *apic = km::GetCpuLocalApic();
    defer { apic->eoi(); };

    km::Scheduler *scheduler = km::GetScheduler();

    if (km::Thread *next = scheduler->getWorkItem()) {
        km::PhysicalAddress cr3 = next->process->ptes.root();
        km::SystemMemory *memory = km::GetSystemMemory();
        memory->getPager().setActiveMap(cr3);

        if (km::Thread *current = km::GetCurrentThread()) {
            current->state = *ctx;
            current->tlsAddress = km::kFsBase.load();
            scheduler->addWorkItem(current);

            km::debug::SendEvent(km::debug::ScheduleTask {
                .previous = uint64_t(current->handleId()),
                .next = uint64_t(next->handleId()),
            });
        } else {
            km::debug::SendEvent(km::debug::ScheduleTask {
                .previous = 0,
                .next = uint64_t(next->handleId()),
            });
        }

        SetCurrentThread(next);
        return next->state;
    } else {
        return *ctx;
    }
}

void km::InstallSchedulerIsr(LocalIsrTable *table) {
    IsrCallback old = table->install(isr::kTimerVector, SchedulerIsr);

    if (old != km::DefaultIsrHandler && old != SchedulerIsr) {
        KmDebugMessage("Failed to install scheduler isr ", (void*)old, " != ", (void*)km::DefaultIsrHandler, "\n");
        KM_PANIC("Failed to install scheduler isr.");
    }
}

void km::ScheduleWork(LocalIsrTable *table, IApic *apic, km::Thread *initial) {
    KmSystemCallStackTlsOffset = tlsSystemCallStack.tlsOffset();

    SetCurrentThread(initial);
    InstallSchedulerIsr(table);

    apic->setTimerDivisor(apic::TimerDivide::e64);
    apic->setInitialCount(0x10000);

    apic->cfgIvtTimer(apic::IvtConfig {
        .vector = isr::kTimerVector,
        .polarity = apic::Polarity::eActiveHigh,
        .trigger = apic::Trigger::eEdge,
        .enabled = true,
        .timer = apic::TimerMode::ePeriodic,
    });

    KmIdle();
}

void km::YieldCurrentThread() {
    IApic *apic = km::GetCpuLocalApic();
    apic->selfIpi(isr::kTimerVector);
}
