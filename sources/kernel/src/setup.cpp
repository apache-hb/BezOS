#include "setup.hpp"
#include "allocator/synchronized.hpp"
#include "allocator/tlsf.hpp"
#include "arch/abi.hpp"
#include "debug/debug.hpp"
#include "debug/packet.hpp"
#include "isr/isr.hpp"
#include "kernel.hpp"
#include "log.hpp"
#include "logger/logger.hpp"
#include "processor.hpp"
#include "system/create.hpp"
#include "system/schedule.hpp"
#include "thread.hpp"
#include "system/process.hpp"

static constexpr bool kEmitAddrToLine = true;
static constexpr stdx::StringView kImagePath = "install/kernel/bin/bezos-limine.elf";

static constinit km::Logger InitLog { "INIT" };

km::PageMemoryTypeLayout km::SetupPat() {
    if (!x64::HasPatSupport()) {
        return PageMemoryTypeLayout { };
    }

    x64::PageAttributeTable pat = x64::PageAttributeTable::get();

    for (uint8_t i = 0; i < pat.count(); i++) {
        km::MemoryType type = pat.getEntry(i);
        InitLog.dbgf("PAT[", i, "]: ", type);
    }

    auto layout = GetDefaultPatLayout();

    pat.setEntry(layout.uncached, MemoryType::eUncached);
    pat.setEntry(layout.writeCombined, MemoryType::eWriteCombine);
    pat.setEntry(layout.writeProtect, MemoryType::eWriteThrough);
    pat.setEntry(layout.writeBack, MemoryType::eWriteBack);
    pat.setEntry(layout.writeProtect, MemoryType::eWriteProtect);
    pat.setEntry(layout.deferred, MemoryType::eUncachedOverridable);

    // PAT[6] and PAT[7] are unused for now, so just set them to UC-
    pat.setEntry(6, MemoryType::eUncachedOverridable);
    pat.setEntry(7, MemoryType::eUncachedOverridable);

    return layout;
}

void km::SetupMtrrs(x64::MemoryTypeRanges& mtrrs, const km::PageBuilder& pm) {
    mtrrs.setDefaultType(km::MemoryType::eWriteBack);

    // Mark all fixed MTRRs as write-back
    if (mtrrs.fixedMtrrSupported()) {
        for (uint8_t i = 0; i < mtrrs.fixedMtrrCount(); i++) {
            mtrrs.setFixedMtrr(i, km::MemoryType::eWriteBack);
        }
    }

    // Disable all variable mtrrs
    for (uint8_t i = 0; i < mtrrs.variableMtrrCount(); i++) {
        mtrrs.setVariableMtrr(i, pm, km::MemoryType::eUncached, nullptr, 0, false);
    }
}

void km::WriteMtrrs(const km::PageBuilder& pm) {
    if (!x64::HasMtrrSupport()) {
        return;
    }

    x64::MemoryTypeRanges mtrrs = x64::MemoryTypeRanges::get();

    InitLog.dbgf("MTRR fixed support: ", present(mtrrs.fixedMtrrSupported()));
    InitLog.dbgf("MTRR fixed enabled: ", enabled(mtrrs.fixedMtrrEnabled()));
    InitLog.dbgf("MTRR fixed count: ", mtrrs.fixedMtrrCount());
    InitLog.dbgf("Default MTRR type: ", mtrrs.defaultType());

    InitLog.dbgf("MTRR variable supported: ", enabled(HasVariableMtrrSupport(mtrrs)));
    InitLog.dbgf("MTRR variable count: ", mtrrs.variableMtrrCount());
    InitLog.dbgf("MTRR write combining: ", enabled(mtrrs.hasWriteCombining()));
    InitLog.dbgf("MTRRs enabled: ", enabled(mtrrs.enabled()));

    if (mtrrs.fixedMtrrSupported()) {
        for (uint8_t i = 0; i < 11; i++) {
            InitLog.print("[INIT] Fixed MTRR[", Int(i).pad(2), "]: ");
            for (uint8_t j = 0; j < 8; j++) {
                if (j != 0) {
                    InitLog.print("| ");
                }

                InitLog.print(mtrrs.fixedMtrr((i * 11) + j), " ");
            }
            InitLog.println();
        }
    }

    if (HasVariableMtrrSupport(mtrrs)) {
        for (uint8_t i = 0; i < mtrrs.variableMtrrCount(); i++) {
            x64::VariableMtrr mtrr = mtrrs.variableMtrr(i);
            InitLog.dbgf("Variable MTRR[", i, "]: ", mtrr.type(), ", address: ", mtrr.baseAddress(pm), ", mask: ", Hex(mtrr.addressMask(pm)).pad(16));
        }
    }

    // SetupMtrrs(mtrrs, pm);
}

void km::WriteMemoryMap(std::span<const boot::MemoryRegion> memmap) {
    InitLog.dbgf(memmap.size(), " memory map entries.");

    InitLog.println("| Entry | Address            | Size               | Type");
    InitLog.println("|-------+--------------------+--------------------+-----------------------");

    for (size_t i = 0; i < memmap.size(); i++) {
        boot::MemoryRegion entry = memmap[i];
        MemoryRange range = entry.range;

        InitLog.println("| ", Int(i).pad(4), "  | ", Hex(range.front.address).pad(16), " | ", rpad(18) + sm::bytes(range.size()), " | ", entry.type);
    }

    InitLog.dbgf("Usable memory: ", boot::UsableMemory(memmap), ", Reclaimable memory: ", boot::ReclaimableMemory(memmap));
}

void km::DumpIsrState(const km::IsrContext *context) {
    KmDebugMessageUnlocked("| Register | Value\n");
    KmDebugMessageUnlocked("|----------+------\n");
    KmDebugMessageUnlocked("| %RAX     | ", Hex(context->rax).pad(16), "\n");
    KmDebugMessageUnlocked("| %RBX     | ", Hex(context->rbx).pad(16), "\n");
    KmDebugMessageUnlocked("| %RCX     | ", Hex(context->rcx).pad(16), "\n");
    KmDebugMessageUnlocked("| %RDX     | ", Hex(context->rdx).pad(16), "\n");
    KmDebugMessageUnlocked("| %RDI     | ", Hex(context->rdi).pad(16), "\n");
    KmDebugMessageUnlocked("| %RSI     | ", Hex(context->rsi).pad(16), "\n");
    KmDebugMessageUnlocked("| %R8      | ", Hex(context->r8).pad(16), "\n");
    KmDebugMessageUnlocked("| %R9      | ", Hex(context->r9).pad(16), "\n");
    KmDebugMessageUnlocked("| %R10     | ", Hex(context->r10).pad(16), "\n");
    KmDebugMessageUnlocked("| %R11     | ", Hex(context->r11).pad(16), "\n");
    KmDebugMessageUnlocked("| %R12     | ", Hex(context->r12).pad(16), "\n");
    KmDebugMessageUnlocked("| %R13     | ", Hex(context->r13).pad(16), "\n");
    KmDebugMessageUnlocked("| %R14     | ", Hex(context->r14).pad(16), "\n");
    KmDebugMessageUnlocked("| %R15     | ", Hex(context->r15).pad(16), "\n");
    KmDebugMessageUnlocked("| %RBP     | ", Hex(context->rbp).pad(16), "\n");
    KmDebugMessageUnlocked("| %RIP     | ", Hex(context->rip).pad(16), "\n");
    KmDebugMessageUnlocked("| %CS      | ", Hex(context->cs).pad(16), "\n");
    KmDebugMessageUnlocked("| %RFLAGS  | ", Hex(context->rflags).pad(16), "\n");
    KmDebugMessageUnlocked("| %RSP     | ", Hex(context->rsp).pad(16), "\n");
    KmDebugMessageUnlocked("| %SS      | ", Hex(context->ss).pad(16), "\n");
    KmDebugMessageUnlocked("| Vector   | ", Hex(context->vector).pad(16), "\n");
    KmDebugMessageUnlocked("| Error    | ", Hex(context->error).pad(16), "\n");
    KmDebugMessageUnlocked("\n");

    if (context->vector == isr::PF) {
        KmDebugMessageUnlocked("| Faulting address | ", Hex(__get_cr2()).pad(16), "\n");

        if (context->error & (1 << 0)) {
            KmDebugMessageUnlocked("PRESENT ");
        } else {
            KmDebugMessageUnlocked("NOT PRESENT ");
        }

        if (context->error & (1 << 1)) {
            KmDebugMessageUnlocked("WRITE ");
        } else {
            KmDebugMessageUnlocked("READ ");
        }

        if (context->error & (1 << 2)) {
            KmDebugMessageUnlocked("USER ");
        } else {
            KmDebugMessageUnlocked("SUPERVISOR ");
        }

        if (context->error & (1 << 3)) {
            KmDebugMessageUnlocked("RESERVED ");
        }

        if (context->error & (1 << 4)) {
            KmDebugMessageUnlocked("FETCH ");
        }

        KmDebugMessageUnlocked("\n");
    }

    if (context->vector != isr::PF || context->error & (1 << 0)) {
        std::byte data[16];
        memcpy(data, (void*)context->rip, sizeof(data));
        KmDebugMessageUnlocked(km::HexDump(data), "\n");
    }

    KmDebugMessageUnlocked("| MSR                 | Value\n");
    KmDebugMessageUnlocked("|---------------------+------\n");
    KmDebugMessageUnlocked("| IA32_GS_BASE        | ", Hex(IA32_GS_BASE.load()).pad(16), "\n");
    KmDebugMessageUnlocked("| IA32_FS_BASE        | ", Hex(IA32_FS_BASE.load()).pad(16), "\n");
    KmDebugMessageUnlocked("| IA32_KERNEL_GS_BASE | ", Hex(IA32_KERNEL_GS_BASE.load()).pad(16), "\n");
    KmDebugMessageUnlocked("\n");
}

void km::DumpCurrentStack() {
    void *rbp = __builtin_frame_address(0);
    PrintStackTrace(rbp);
}

void km::PrintStackTrace(const void *rbp) {
    KmDebugMessageUnlocked("|----Stack trace-----+----------------\n");
    KmDebugMessageUnlocked("| Frame              | Program Counter\n");
    KmDebugMessageUnlocked("|--------------------+----------------\n");

    if (SystemMemory *memory = km::GetSystemMemory()) {
        x64::WalkStackFramesChecked(memory->systemTables(), (void**)rbp, [](void **frame, void *pc, stdx::StringView note) {
            KmDebugMessageUnlocked("| ", (void*)frame, " | ", pc);
            if (!note.isEmpty()) {
                KmDebugMessageUnlocked(" ", note);
            }
            KmDebugMessageUnlocked("\n");
        });
    } else {
        x64::WalkStackFrames((void**)rbp, [](void **frame, void *pc) {
            KmDebugMessageUnlocked("| ", (void*)frame, " | ", pc);
            KmDebugMessageUnlocked("\n");
        });
    }

    if (kEmitAddrToLine) {
        KmDebugMessageUnlocked("llvm-addr2line -e ", kImagePath);

        if (SystemMemory *memory = km::GetSystemMemory()) {
            x64::WalkStackFramesChecked(memory->systemTables(), (void**)rbp, [](void **, void *pc, stdx::StringView) {
                KmDebugMessageUnlocked(" ", pc);
            });
        } else {
            x64::WalkStackFrames((void**)rbp, [](void **, void *pc) {
                KmDebugMessageUnlocked(" ", pc);
            });
        }

        KmDebugMessageUnlocked("\n");
    }
}

void km::DumpStackTrace(const km::IsrContext *context) {
    PrintStackTrace((void*)context->rbp);
}

void km::DumpIsrContext(const km::IsrContext *context, stdx::StringView message) {
    if (km::IsCpuStorageSetup()) {
        KmDebugMessageUnlocked("\n[BUG] ", message, " - ", km::GetCurrentCoreId(), "\n");
    } else {
        KmDebugMessageUnlocked("\n[BUG] ", message, "\n");
    }

    DumpIsrState(context);
}

static bool IsSupervisorFault(const km::IsrContext *context) {
    return (context->cs & 0b11) == 0;
}

static void FaultProcess(km::IsrContext *context, stdx::StringView fault) {
    if (auto process = sys::GetCurrentProcess()) {
        KmDebugMessageUnlocked("[PROC] Terminating ", process->getName(), ", due to ", fault, "\n");
        process->destroy(km::GetSysSystem(), sys::ProcessDestroyInfo { .exitCode = 0, .reason = eOsProcessFaulted });
        DumpIsrState(context);
        DumpStackTrace(context);

        sys::YieldCurrentThread();
    }
}

void km::InstallExceptionHandlers(SharedIsrTable *ist) {
    ist->install(isr::DE, [](km::IsrContext *context) noexcept [[clang::reentrant]] -> km::IsrContext {
        if (!IsSupervisorFault(context)) {
            FaultProcess(context, "divide by zero (#DE)");
            return *context;
        }

        DumpIsrContext(context, "Divide by zero (#DE)");
        DumpStackTrace(context);
        KM_PANIC("Kernel panic.");
    });

    ist->install(isr::NMI, [](km::IsrContext *context) noexcept [[clang::reentrant]] -> km::IsrContext {
        KmDebugMessageUnlocked("[INT] Non-maskable interrupt (#NM)\n");
        DumpIsrState(context);
        return *context;
    });

    ist->install(isr::UD, [](km::IsrContext *context) noexcept [[clang::reentrant]] -> km::IsrContext {
        if (!IsSupervisorFault(context)) {
            FaultProcess(context, "invalid opcode (#UD)");
            return *context;
        }

        DumpIsrContext(context, "Invalid opcode (#UD)");
        DumpStackTrace(context);
        KM_PANIC("Kernel panic.");
    });

    ist->install(isr::DF, [](km::IsrContext *context) noexcept [[clang::reentrant]] -> km::IsrContext {
        if (!IsSupervisorFault(context)) {
            FaultProcess(context, "double fault (#DF)");
            return *context;
        }

        DumpIsrContext(context, "Double fault (#DF)");
        DumpStackTrace(context);
        KM_PANIC("Kernel panic.");
    });

    ist->install(isr::GP, [](km::IsrContext *context) noexcept [[clang::reentrant]] -> km::IsrContext {
        NmiGuard guard;

        if (!IsSupervisorFault(context)) {
            FaultProcess(context, "general protection fault (#GP)");
            return *context;
        }

        DumpIsrContext(context, "General protection fault (#GP)");
        DumpStackTrace(context);
        KM_PANIC("Kernel panic.");
    });

    ist->install(isr::PF, [](km::IsrContext *context) noexcept [[clang::reentrant]] -> km::IsrContext {
        if (!IsSupervisorFault(context)) {
            FaultProcess(context, "page fault (#PF)");
            return *context;
        }

        KmDebugMessageUnlocked("[BUG] CR2: ", Hex(__get_cr2()).pad(16), "\n");
        DumpIsrContext(context, "Page fault (#PF)");
        DumpStackTrace(context);
        KM_PANIC("Kernel panic.");
    });
}

using TlsfAllocatorSync = mem::SynchronizedAllocator<mem::TlsfAllocator>;

static constinit mem::IAllocator *gAllocator = nullptr;

extern "C" void *malloc(size_t size) noexcept {
    void *ptr = gAllocator->allocate(size);
    km::debug::SendEvent(km::debug::AllocateVirtualMemory {
        .size = size,
        .address = (uintptr_t)ptr,
        .alignment = 0,
        .tag = 0,
    });
    return ptr;
}

extern "C" void *realloc(void *old, size_t size) noexcept {
    return gAllocator->reallocate(old, 0, size);
}

__attribute__((__nothrow__, __nonnull__, __nonallocating__))
extern "C" void free(void *ptr) {
    km::debug::SendEvent(km::debug::ReleaseVirtualMemory {
        .begin = (uintptr_t)ptr,
        .end = (uintptr_t)ptr + tlsf_block_size(ptr) - tlsf_alloc_overhead(),
        .tag = 0,
    });

    gAllocator->deallocate(ptr, 0);
}

__attribute__((__nothrow__, __malloc__, __alloc_size__(2), __alloc_align__(1), __allocating__))
extern "C" void *aligned_alloc(size_t alignment, size_t size) noexcept {
    void *ptr = gAllocator->allocateAligned(size, alignment);
    km::debug::SendEvent(km::debug::AllocateVirtualMemory {
        .size = size,
        .address = (uintptr_t)ptr,
        .alignment = static_cast<uint32_t>(alignment),
        .tag = 0,
    });
    return ptr;
}

void km::InitGlobalAllocator(void *memory, size_t size) {
    mem::TlsfAllocator alloc{memory, size};
    void *mem = alloc.allocateAligned(sizeof(TlsfAllocatorSync), alignof(TlsfAllocatorSync));
    gAllocator = new (mem) TlsfAllocatorSync(std::move(alloc));
}
