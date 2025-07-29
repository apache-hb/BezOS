#include "apic.hpp"
#include "gdt.h"
#include "thread.hpp"

#include "isr/isr.hpp"

#include "system/schedule.hpp"
#include "system/thread.hpp"

#include "task/scheduler.hpp"
#include "task/scheduler_queue.hpp"
#include "task/runtime.hpp"

extern "C" [[noreturn]] void __x86_64_resume(km::IsrContext *context);

extern "C" uint64_t KmSystemCallStackTlsOffset;

static task::Scheduler *gScheduler;

CPU_LOCAL
static constinit km::CpuLocal<void*> tlsKernelStack;

CPU_LOCAL
static constinit km::CpuLocal<task::SchedulerQueue*> tlsQueue;

void sys::setupGlobalScheduler(bool enableSmp, acpi::AcpiTables& rsdt) {
    gScheduler = new task::Scheduler;
    task::Scheduler::create(gScheduler);
    size_t cpuCount = enableSmp ? rsdt.madt()->lapicCount() : 1;
    task::SchedulerQueue *queues = new task::SchedulerQueue[cpuCount];

    if (!enableSmp) {
        task::SchedulerQueue::create(64, &queues[0]);
        gScheduler->addQueue(km::GetCurrentCoreId(), &queues[0]);
        tlsQueue = &queues[0];
    } else {
        for (const acpi::MadtEntry *madt : *rsdt.madt()) {
            if (madt->type != acpi::MadtEntryType::eLocalApic)
                continue;

            const acpi::MadtEntry::LocalApic localApic = madt->apic;

            if (!localApic.isEnabled() && !localApic.isOnlineCapable()) {
                continue;
            }

            task::SchedulerQueue& queue = queues[localApic.apicId];
            task::SchedulerQueue::create(64, &queue);

            gScheduler->addQueue(km::CpuCoreId(localApic.apicId), &queue);
        }

        tlsQueue = gScheduler->getQueue(km::GetCurrentCoreId());
    }
}

void sys::setupApScheduler() {
    tlsQueue = gScheduler->getQueue(km::GetCurrentCoreId());
}

task::Scheduler *sys::getScheduler() {
    return gScheduler;
}

task::SchedulerQueue *sys::getTlsQueue() {
    return tlsQueue.get();
}

void sys::installSchedulerIsr() {
    KmSystemCallStackTlsOffset = tlsKernelStack.tlsOffset();
    km::SharedIsrTable *sharedIst = km::GetSharedIsrTable();
    sharedIst->install(km::isr::kTimerVector, [](km::IsrContext *isrContext) noexcept [[clang::reentrant]] -> km::IsrContext {
        km::IApic *apic = km::GetCpuLocalApic();

        task::SchedulerQueue *queue = tlsQueue.get();
        if (task::switchCurrentContext(gScheduler, queue, isrContext)) {
            // task::SchedulerEntry *entry = queue->getCurrentTask();
            // sys::Thread *thread = static_cast<sys::Thread*>(entry);
            // thread->loadState();
        }

        apic->eoi();
        return *isrContext;
    });
}

sm::RcuSharedPtr<sys::Process> sys::GetCurrentProcess() {
    auto queue = tlsQueue.get();
    auto entry = queue->getCurrentTask();
    if (entry != nullptr) {
        auto thread = static_cast<sys::Thread*>(entry);
        return thread->getProcess();
    }

    return nullptr;
}

sm::RcuSharedPtr<sys::Thread> sys::GetCurrentThread() {
    auto queue = tlsQueue.get();
    auto entry = queue->getCurrentTask();
    if (entry != nullptr) {
        auto thread = static_cast<sys::Thread*>(entry);
        return thread->loanShared();
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

    task::switchCurrentContext(gScheduler, tlsQueue.get(), &context);

    if ((context.cs & 0b11) != 0) {
        __swapgs();
    }

    __x86_64_resume(&context);
}
