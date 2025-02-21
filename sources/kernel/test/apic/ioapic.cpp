#include <gtest/gtest.h>

#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stddef.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/prctl.h>

#include "apic.hpp"
#include "util/defer.hpp"
#include "test/test_memory.hpp"

#include <capstone/capstone.h>

// taken from https://github.com/hawkinsw/hwbp_lib/tree/master

enum {
	BREAK_EXEC = 0x0,
	BREAK_WRITE = 0x1,
	BREAK_READWRITE = 0x3,
};

enum {
	BREAK_ONE = 0x0,
	BREAK_TWO = 0x1,
	BREAK_FOUR = 0x3,
	BREAK_EIGHT = 0x2,
};

#define ENABLE_BREAKPOINT(x) (0x1<<(x*2))
#define ENABLE_BREAK_EXEC(x) (BREAK_EXEC<<(16+(x*4)))
#define ENABLE_BREAK_WRITE(x) (BREAK_WRITE<<(16+(x*4)))
#define ENABLE_BREAK_READWRITE(x) (BREAK_READWRITE<<(16+(x*4)))

/*
 * This function fork()s a child that will use
 * ptrace to set a hardware breakpoint for
 * memory r/w at _addr_. When the breakpoint is
 * hit, then _handler_ is invoked in a signal-
 * handling context.
 */
bool install_breakpoint(void *addr, int bpno, void (*handler)(int)) {
	pid_t child = 0;
	uint32_t enable_breakpoint = ENABLE_BREAKPOINT(bpno);
	uint32_t enable_breakwrite = ENABLE_BREAK_WRITE(bpno);
	pid_t parent = getpid();
	int child_status = 0;

	if (!(child = fork()))
	{
		int parent_status = 0;
		if (ptrace(PTRACE_ATTACH, parent, NULL, NULL))
			_exit(1);

		while (!WIFSTOPPED(parent_status))
			waitpid(parent, &parent_status, 0);

		/*
		 * set the breakpoint address.
		 */
		if (ptrace(PTRACE_POKEUSER,
		           parent,
		           offsetof(struct user, u_debugreg[bpno]),
		           addr))
			_exit(1);

		/*
		 * set parameters for when the breakpoint should be triggered.
		 */
		if (ptrace(PTRACE_POKEUSER,
		           parent,
		           offsetof(struct user, u_debugreg[7]),
		           enable_breakwrite | enable_breakpoint))
			_exit(1);

		if (ptrace(PTRACE_DETACH, parent, NULL, NULL))
			_exit(1);

		_exit(0);
	}

	waitpid(child, &child_status, 0);

	signal(SIGTRAP, handler);

	if (WIFEXITED(child_status) && !WEXITSTATUS(child_status))
		return true;
	return false;
}

/*
 * This function will disable a breakpoint by
 * invoking install_breakpoint is a 0x0 _addr_
 * and no handler function. See comments above
 * for implementation details.
 */
bool disable_breakpoint(int bpno)
{
	return install_breakpoint(0x0, bpno, NULL);
}

#if 0
TEST(IoApicTest, Construct) {
    SystemMemoryTestBody body;
    body.addSegment(0x100000, boot::MemoryRegion::eUsable);

    auto memory = body.make();

    //
    // ACPI tables only have 32 bits of address space for the ioapic base.
    // To emulate this we need to allocate memory below the 4g limit.
    //

    constexpr size_t kIoApicSize = 0x20;
    void *ioapicMemory = mmap(
        nullptr,
        kIoApicSize,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT,
        0, 0
    );

    ASSERT_NE(ioapicMemory, MAP_FAILED);
    uintptr_t ioapicAddress = (uintptr_t)ioapicMemory;
    ASSERT_LT(ioapicAddress, sm::gigabytes(4).bytes());

    uint32_t *ioapic = (uint32_t*)ioapicMemory;
    memset(ioapic, 0x0, kIoApicSize);

    defer { munmap(ioapicMemory, kIoApicSize); };

    acpi::MadtEntry entry {
        .type = acpi::MadtEntryType::eIoApic,
        .length = sizeof(acpi::MadtEntry::IoApic) + 0x2,
        .ioapic = acpi::MadtEntry::IoApic {
            .ioApicId = 0,
            .address = (uint32_t)ioapicAddress,
            .interruptBase = 0,
        }
    };

    km::IoApic object(&entry, memory);
    ASSERT_TRUE(object.present());
    ASSERT_TRUE(memory.pager.isHigherHalf(object.address()));
}
#endif

static uint8_t *gIoApicMemory = nullptr;
static csh gCapstoneHandle;
static int gTestLogFd = -1;

struct IoApicRegisters {
    uint8_t regs[0x100];

    uint32_t select;
};

static IoApicRegisters gIoApic{};

static uint64_t RegisterValue(const mcontext_t *mc, x86_reg reg) {
    switch (reg) {
    case X86_REG_RAX: return mc->gregs[REG_RAX];
    case X86_REG_RBX: return mc->gregs[REG_RBX];
    case X86_REG_RCX: return mc->gregs[REG_RCX];
    case X86_REG_RDX: return mc->gregs[REG_RDX];
    case X86_REG_RSI: return mc->gregs[REG_RSI];
    case X86_REG_RDI: return mc->gregs[REG_RDI];
    case X86_REG_RBP: return mc->gregs[REG_RBP];
    case X86_REG_RSP: return mc->gregs[REG_RSP];
    case X86_REG_R8: return mc->gregs[REG_R8];
    case X86_REG_R9: return mc->gregs[REG_R9];
    case X86_REG_R10: return mc->gregs[REG_R10];
    case X86_REG_R11: return mc->gregs[REG_R11];
    case X86_REG_R12: return mc->gregs[REG_R12];
    case X86_REG_R13: return mc->gregs[REG_R13];
    case X86_REG_R14: return mc->gregs[REG_R14];
    case X86_REG_R15: return mc->gregs[REG_R15];
    case X86_REG_RIP: return mc->gregs[REG_RIP];
    default: return 0;
    }
}

static uint64_t EvalMemoryOperand(const mcontext_t *mc, const x86_op_mem *mem) {
    uint64_t offset = mem->disp;
    uint64_t base = RegisterValue(mc, mem->base);
    uint64_t index = RegisterValue(mc, mem->index);
    return base + (index * mem->scale) + offset;
}

static uint64_t IoApicSelect() {
    return (uint64_t)gIoApicMemory + 0x0;
}

static uint64_t IoApicWindow() {
    return (uint64_t)gIoApicMemory + 0x10;
}

constexpr struct sigaction kSegvHandler {
    .sa_sigaction = [](int, siginfo_t *, void *ucontext) {
        //
        // Get the current RIP and disassemble the instruction.
        //
        ucontext_t *uc = (ucontext_t*)ucontext;
        mcontext_t mc = uc->uc_mcontext;
        void *rip = (void*)mc.gregs[REG_RIP];

        //
        // While calling stuff that invokes malloc is technically not kosher
        // in a signal handler. We know that the code that raises this signal
        // handler does not perform any signal unsafe operations, so we can
        // get away with it.
        //
        cs_insn insn;
        uint64_t address = (uint64_t)rip;
        bool ok = cs_disasm_iter(gCapstoneHandle, (const uint8_t**)&rip, (size_t*)&rip, &address, &insn);
        if (!ok) {
            dprintf(gTestLogFd, "Failed to disassemble instruction at %p\n", rip);
            abort();
        }

        //
        // Ensure that the fault is caused by a read or write to the ioapic.
        //

        cs_detail *detail = insn.detail;
        cs_x86 *x86 = &detail->x86;
        cs_x86_op *op = x86->operands;
        for (uint8_t i = 0; i < x86->op_count; i++) {
            if (op[i].type == X86_OP_MEM) {
                uint64_t address = EvalMemoryOperand(&mc, &op[i].mem);
                if (address == IoApicSelect() && op[i].access == CS_AC_WRITE) {
                    //
                    // We've received a write to the select register.
                    // To fullfil the request we need to read the value written
                    // and then use that to select a register.
                    //
                    return;
                }
            }
        }
        abort();
    },
    .sa_flags = SA_SIGINFO,
};

constexpr struct sigaction kTrapHandler {
    .sa_sigaction = [](int, siginfo_t *, void *ucontext) {

    },
    .sa_flags = SA_SIGINFO,
};

TEST(IoApicTest, InputCount) {
    SystemMemoryTestBody body;
    body.addSegment(0x100000, boot::MemoryRegion::eUsable);

    auto memory = body.make();

    gTestLogFd = open("./ioapic.log", O_WRONLY | O_TRUNC | O_CREAT);
    ASSERT_NE(gTestLogFd, -1);
    defer { close(gTestLogFd); };

    cs_err cserr = cs_open(CS_ARCH_X86, CS_MODE_64, &gCapstoneHandle);
    ASSERT_EQ(cserr, CS_ERR_OK);

    cs_option(gCapstoneHandle, CS_OPT_DETAIL, CS_OPT_ON);
    cs_option(gCapstoneHandle, CS_OPT_SKIPDATA, CS_OPT_OFF);

    defer { cs_close(&gCapstoneHandle); };

    //
    // ACPI tables only have 32 bits of address space for the ioapic base.
    // To emulate this we need to allocate memory below the 4g limit.
    //

    constexpr size_t kIoApicSize = 0x20;
    gIoApicMemory = (uint8_t*)mmap(
        nullptr,
        kIoApicSize,
        PROT_NONE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT,
        0, 0
    );

    ASSERT_NE(gIoApicMemory, MAP_FAILED);
    uintptr_t ioapicAddress = (uintptr_t)gIoApicMemory;
    ASSERT_LT(ioapicAddress, sm::gigabytes(4).bytes());

    defer { munmap(gIoApicMemory, kIoApicSize); };

    int err = sigaction(SIGSEGV, &kSegvHandler, nullptr);
    ASSERT_EQ(err, 0);
    defer { sigaction(SIGSEGV, nullptr, nullptr); };

    err = sigaction(SIGTRAP, &kTrapHandler, nullptr);
    ASSERT_EQ(err, 0);
    defer { sigaction(SIGTRAP, nullptr, nullptr); };

    acpi::MadtEntry entry {
        .type = acpi::MadtEntryType::eIoApic,
        .length = sizeof(acpi::MadtEntry::IoApic) + 0x2,
        .ioapic = acpi::MadtEntry::IoApic {
            .ioApicId = 0,
            .address = (uint32_t)ioapicAddress,
            .interruptBase = 0,
        }
    };

    km::IoApic object(&entry, memory);
    ASSERT_TRUE(object.present());
    ASSERT_TRUE(memory.pager.isHigherHalf(object.address()));
}