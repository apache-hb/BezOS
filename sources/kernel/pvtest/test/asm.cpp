#include <gtest/gtest.h>

#include "pvtest/pvtest.hpp"

#include "common/compiler/compiler.hpp"

bool gContextSwitched = false;

[[noreturn]]
void ContextSwitchHandler(void *rip, void *rsp, void *rbp) {
    gContextSwitched = true;
    asm volatile(
        "mov $1, %%rdi\n"
        "mov %0, %%rsp\n"
        "mov %1, %%rbp\n"
        "jmpq *%2\n"
        :: "r"(rsp), "r"(rbp), "r"(rip)
    );
    std::unreachable();
}

void ContextSwitchTestBody(int arg) {
    if (arg != 0) return;

    CLANG_DIAGNOSTIC_PUSH();
    CLANG_DIAGNOSTIC_IGNORE("-Wframe-address");

    uintptr_t rbp = (uintptr_t)__builtin_frame_address(1);
    uintptr_t rsp = (uintptr_t)__builtin_frame_address(0);

    CLANG_DIAGNOSTIC_POP();

    void *stack = malloc(0x2000);
    gregset_t gregs{};
    gregs[REG_RDI] = (uintptr_t)&ContextSwitchTestBody;
    gregs[REG_RSI] = rsp + 0x8;
    gregs[REG_RDX] = rbp + 0x8;
    gregs[REG_RSP] = (uintptr_t)stack + 0x2000;
    gregs[REG_RBP] = (uintptr_t)stack + 0x2000;
    gregs[REG_RIP] = (uintptr_t)&ContextSwitchHandler;

    // Save preserved registers
    asm volatile(
        "mov %%rbx, %[grbx]\n"
        "mov %%r12, %[gr12]\n"
        "mov %%r13, %[gr13]\n"
        "mov %%r14, %[gr14]\n"
        "mov %%r15, %[gr15]\n"
        : [grbx]"=r"(gregs[REG_RBX]),
          [gr12]"=r"(gregs[REG_R12]),
          [gr13]"=r"(gregs[REG_R13]),
          [gr14]"=r"(gregs[REG_R14]),
          [gr15]"=r"(gregs[REG_R15])
    );

    PvTestContextSwitch(&gregs);
}

TEST(PvAsmTest, ContextSwitch) {
    ContextSwitchTestBody(0);

    ASSERT_TRUE(gContextSwitched) << "Context switch failed";
}
