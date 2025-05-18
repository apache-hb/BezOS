#include <stdint.h>

#include "arch/abi.hpp"
#include "kernel.hpp"
#include "logger/categories.hpp"

static constexpr bool kEmitAddrToLine = true;
static constexpr stdx::StringView kImagePath = "install/kernel/bin/bezos-limine.elf";

extern "C" [[gnu::weak]] uintptr_t __stack_chk_guard = 0xDEADBEEF0D0D0D0D;

extern "C"
[[gnu::weak, noreturn, gnu::section(".text.stackchk"), gnu::no_stack_protector]]
void __stack_chk_fail() {
    if (km::SystemMemory *memory = km::GetSystemMemory()) {
        void *rbp = __builtin_frame_address(0);
        x64::WalkStackFramesChecked(memory->systemTables(), (void**)rbp, [](void **frame, void *pc, stdx::StringView note) {
            InitLog.print("| ", (void*)frame, " | ", pc);
            if (!note.isEmpty()) {
                InitLog.print(" ", note);
            }
            InitLog.println();
        });


        if (kEmitAddrToLine) {
            InitLog.print("llvm-addr2line -e ", kImagePath);

            x64::WalkStackFramesChecked(memory->systemTables(), (void**)rbp, [](void **, void *pc, stdx::StringView) {
                InitLog.print(" ", pc);
            });

            InitLog.println();
        }
    }

    KM_PANIC("Stack smashing detected");
}
