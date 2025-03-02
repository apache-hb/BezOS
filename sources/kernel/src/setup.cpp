#include "setup.hpp"
#include "allocator/synchronized.hpp"
#include "allocator/tlsf.hpp"
#include "arch/abi.hpp"
#include "kernel.hpp"
#include "log.hpp"
#include "processor.hpp"
#include "thread.hpp"

static constexpr bool kEmitAddrToLine = true;
static constexpr stdx::StringView kImagePath = "install/kernel/bin/bezos-limine.elf";

km::PageMemoryTypeLayout km::SetupPat() {
    if (!x64::HasPatSupport()) {
        return PageMemoryTypeLayout { };
    }

    x64::PageAttributeTable pat = x64::PageAttributeTable::get();

    for (uint8_t i = 0; i < pat.count(); i++) {
        km::MemoryType type = pat.getEntry(i);
        KmDebugMessage("[INIT] PAT[", i, "]: ", type, "\n");
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

    KmDebugMessage("[INIT] MTRR fixed support: ", present(mtrrs.fixedMtrrSupported()), "\n");
    KmDebugMessage("[INIT] MTRR fixed enabled: ", enabled(mtrrs.fixedMtrrEnabled()), "\n");
    KmDebugMessage("[INIT] MTRR fixed count: ", mtrrs.fixedMtrrCount(), "\n");
    KmDebugMessage("[INIT] Default MTRR type: ", mtrrs.defaultType(), "\n");

    KmDebugMessage("[INIT] MTRR variable supported: ", enabled(HasVariableMtrrSupport(mtrrs)), "\n");
    KmDebugMessage("[INIT] MTRR variable count: ", mtrrs.variableMtrrCount(), "\n");
    KmDebugMessage("[INIT] MTRR write combining: ", enabled(mtrrs.hasWriteCombining()), "\n");
    KmDebugMessage("[INIT] MTRRs enabled: ", enabled(mtrrs.enabled()), "\n");

    if (mtrrs.fixedMtrrSupported()) {
        for (uint8_t i = 0; i < 11; i++) {
            KmDebugMessage("[INIT] Fixed MTRR[", rpad(2) + i, "]: ");
            for (uint8_t j = 0; j < 8; j++) {
                if (j != 0) {
                    KmDebugMessage("| ");
                }

                KmDebugMessage(mtrrs.fixedMtrr((i * 11) + j), " ");
            }
            KmDebugMessage("\n");
        }
    }

    if (HasVariableMtrrSupport(mtrrs)) {
        for (uint8_t i = 0; i < mtrrs.variableMtrrCount(); i++) {
            x64::VariableMtrr mtrr = mtrrs.variableMtrr(i);
            KmDebugMessage("[INIT] Variable MTRR[", i, "]: ", mtrr.type(), ", address: ", mtrr.baseAddress(pm), ", mask: ", Hex(mtrr.addressMask(pm)).pad(16, '0'), "\n");
        }
    }

    // SetupMtrrs(mtrrs, pm);
}

void km::WriteMemoryMap(std::span<const boot::MemoryRegion> memmap) {
    KmDebugMessage("[INIT] ", memmap.size(), " memory map entries.\n");

    KmDebugMessage("| Entry | Address            | Size               | Type\n");
    KmDebugMessage("|-------+--------------------+--------------------+-----------------------\n");

    for (size_t i = 0; i < memmap.size(); i++) {
        boot::MemoryRegion entry = memmap[i];
        MemoryRange range = entry.range;

        KmDebugMessage("| ", Int(i).pad(4, '0'), "  | ", Hex(range.front.address).pad(16, '0'), " | ", rpad(18) + sm::bytes(range.size()), " | ", entry.type, "\n");
    }

    KmDebugMessage("[INIT] Usable memory: ", boot::UsableMemory(memmap), ", Reclaimable memory: ", boot::ReclaimableMemory(memmap), "\n");
}

void km::DumpIsrState(const km::IsrContext *context) {
    KmDebugMessageUnlocked("| Register | Value\n");
    KmDebugMessageUnlocked("|----------+------\n");
    KmDebugMessageUnlocked("| %RAX     | ", Hex(context->rax).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %RBX     | ", Hex(context->rbx).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %RCX     | ", Hex(context->rcx).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %RDX     | ", Hex(context->rdx).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %RDI     | ", Hex(context->rdi).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %RSI     | ", Hex(context->rsi).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %R8      | ", Hex(context->r8).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %R9      | ", Hex(context->r9).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %R10     | ", Hex(context->r10).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %R11     | ", Hex(context->r11).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %R12     | ", Hex(context->r12).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %R13     | ", Hex(context->r13).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %R14     | ", Hex(context->r14).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %R15     | ", Hex(context->r15).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %RBP     | ", Hex(context->rbp).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %RIP     | ", Hex(context->rip).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %CS      | ", Hex(context->cs).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %RFLAGS  | ", Hex(context->rflags).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %RSP     | ", Hex(context->rsp).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| %SS      | ", Hex(context->ss).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| Vector   | ", Hex(context->vector).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| Error    | ", Hex(context->error).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("\n");

    if (context->vector == isr::PF) {
        KmDebugMessageUnlocked("| Faulting address | ", Hex(__get_cr2()).pad(16, '0'), "\n");

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

    if (context->vector == isr::UD) {
        std::byte data[16];
        memcpy(data, (void*)context->rip, sizeof(data));
        KmDebugMessageUnlocked(km::HexDump(data));
        KmDebugMessageUnlocked("\n");
    }

    KmDebugMessageUnlocked("| MSR                 | Value\n");
    KmDebugMessageUnlocked("|---------------------+------\n");
    KmDebugMessageUnlocked("| IA32_GS_BASE        | ", Hex(kGsBase.load()).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| IA32_FS_BASE        | ", Hex(kFsBase.load()).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("| IA32_KERNEL_GS_BASE | ", Hex(kKernelGsBase.load()).pad(16, '0'), "\n");
    KmDebugMessageUnlocked("\n");
}

void km::DumpStackTrace(const km::IsrContext *context) {
    KmDebugMessageUnlocked("|----Stack trace-----+----------------\n");
    KmDebugMessageUnlocked("| Frame              | Program Counter\n");
    KmDebugMessageUnlocked("|--------------------+----------------\n");
    if (SystemMemory *memory = km::GetSystemMemory()) {
        x64::WalkStackFramesChecked(memory->systemTables(), (void**)context->rbp, [](void **frame, void *pc, stdx::StringView note) {
            KmDebugMessageUnlocked("| ", (void*)frame, " | ", pc);
            if (!note.isEmpty()) {
                KmDebugMessageUnlocked(" ", note);
            }
            KmDebugMessageUnlocked("\n");
        });
    }

    if (kEmitAddrToLine) {
        KmDebugMessageUnlocked("llvm-addr2line -e ", kImagePath);

        if (SystemMemory *memory = km::GetSystemMemory()) {
            x64::WalkStackFramesChecked(memory->systemTables(), (void**)context->rbp, [](void **, void *pc, stdx::StringView) {
                KmDebugMessageUnlocked(" ", pc);
            });
        }

        KmDebugMessageUnlocked("\n");
    }
}

void km::DumpIsrContext(const km::IsrContext *context, stdx::StringView message) {
    if (km::IsCpuStorageSetup()) {
        KmDebugMessageUnlocked("\n[BUG] ", message, " - ", km::GetCurrentCoreId(), "\n");
    } else {
        KmDebugMessageUnlocked("\n[BUG] ", message, "\n");
    }

    DumpIsrState(context);
}

void km::InstallExceptionHandlers(SharedIsrTable *ist) {
    ist->install(isr::DE, [](km::IsrContext *context) -> km::IsrContext {
        DumpIsrContext(context, "Divide by zero (#DE)");
        DumpStackTrace(context);
        KM_PANIC("Kernel panic.");
    });

    ist->install(isr::NMI, [](km::IsrContext *context) -> km::IsrContext {
        KmDebugMessageUnlocked("[INT] Non-maskable interrupt (#NM)\n");
        DumpIsrState(context);
        return *context;
    });

    ist->install(isr::UD, [](km::IsrContext *context) -> km::IsrContext {
        DumpIsrContext(context, "Invalid opcode (#UD)");
        DumpStackTrace(context);
        KM_PANIC("Kernel panic.");
    });

    ist->install(isr::DF, [](km::IsrContext *context) -> km::IsrContext {
        DumpIsrContext(context, "Double fault (#DF)");
        DumpStackTrace(context);
        KM_PANIC("Kernel panic.");
    });

    ist->install(isr::GP, [](km::IsrContext *context) -> km::IsrContext {
        NmiGuard guard;

        DumpIsrContext(context, "General protection fault (#GP)");
        DumpStackTrace(context);
        KM_PANIC("Kernel panic.");
    });

    ist->install(isr::PF, [](km::IsrContext *context) -> km::IsrContext {
        KmDebugMessageUnlocked("[BUG] CR2: ", Hex(__get_cr2()).pad(16, '0'), "\n");
        DumpIsrContext(context, "Page fault (#PF)");
        DumpStackTrace(context);
        KM_PANIC("Kernel panic.");
    });
}

using TlsfAllocatorSync = mem::SynchronizedAllocator<mem::TlsfAllocator>;

static constinit mem::IAllocator *gAllocator = nullptr;

extern "C" void *malloc(size_t size) {
    return gAllocator->allocate(size);
}

extern "C" void *realloc(void *old, size_t size) {
    return gAllocator->reallocate(old, 0, size);
}

extern "C" void free(void *ptr) {
    gAllocator->deallocate(ptr, 0);
}

extern "C" void *aligned_alloc(size_t alignment, size_t size) {
    return gAllocator->allocateAligned(size, alignment);
}

void km::InitGlobalAllocator(void *memory, size_t size) {
    mem::TlsfAllocator alloc{memory, size};
    void *mem = alloc.allocateAligned(sizeof(TlsfAllocatorSync), alignof(TlsfAllocatorSync));
    gAllocator = new (mem) TlsfAllocatorSync(std::move(alloc));
}
