#include <gtest/gtest.h>

#include "pvtest/pvtest.hpp"

bool gContextSwitched = false;

[[noreturn]]
void ContextSwitchHandler(void *ret, void *oldStack) {
    gContextSwitched = true;
    asm volatile(
        "mov $1, %%rdi\n"
        "mov %0, %%rsp\n"
        "mov %1, %%rbp\n"
        "jmpq *%2\n"
        :: "r"(oldStack), "r"(oldStack), "r"(ret)
    );
    std::unreachable();
}

void ContextSwitchTestBody(int arg) {
    if (arg != 0) return;

    void *stack = malloc(0x2000);
    gregset_t gregs{};
    gregs[REG_RDI] = (uintptr_t)__builtin_return_address(0);
    gregs[REG_RSI] = (uintptr_t)__builtin_frame_address(0);
    gregs[REG_RSP] = (uintptr_t)stack + 0x2000 - 0x8;
    gregs[REG_RBP] = (uintptr_t)stack + 0x2000 - 0x8;
    gregs[REG_RIP] = (uintptr_t)&ContextSwitchHandler;

    PvTestContextSwitch(&gregs);
}

TEST(PvAsmTest, ContextSwitch) {
    ContextSwitchTestBody(0);

    ASSERT_TRUE(gContextSwitched) << "Context switch failed";
}
