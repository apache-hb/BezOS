#include "ktest.hpp"
#include "absl/container/flat_hash_set.h"
#include "log.hpp"
#include "util/defer.hpp"
#include "util/format.hpp"

#include <capstone.h>

#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/ucontext.h>
#include <sys/user.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <bits/syscall.h>

#if __has_include(<valgrind/memcheck.h>)
#	include <valgrind/memcheck.h>
#else
#	define VALGRIND_MAKE_MEM_DEFINED(...)
#endif

extern "C" void __gcov_reset(void);

[[noreturn, gnu::no_instrument_function]]
void exit2(int status) {
	__gcov_reset();
	_exit(status);
}

static kmtest::Machine *gMachine = nullptr;
static csh gCapstone;

enum {
    eAccessRead = 0x1,
    eAccessWrite = 0x2,
};

static thread_local inline kmtest::MmioRegion *mCurrentMmioRegion = nullptr;
static thread_local inline int gMmioAccess = 0;
static thread_local inline uintptr_t gMmioOffset = 0;

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
[[gnu::no_instrument_function]]
bool breakpoint(void *addr, int bpno, void (*handler)(int)) {
	pid_t child = 0;
	uint32_t enable_breakpoint = ENABLE_BREAKPOINT(bpno);
	uint32_t enable_breakwrite = ENABLE_BREAK_EXEC(bpno);
	pid_t parent = getpid();
	int child_status = 0;

	if (!(child = fork()))
	{
		int parent_status = 0;
		if (ptrace(PTRACE_ATTACH, parent, NULL, NULL))
			exit2(1);

		while (!WIFSTOPPED(parent_status))
			waitpid(parent, &parent_status, 0);

		/*
		 * set the breakpoint address.
		 */
		if (ptrace(PTRACE_POKEUSER,
		           parent,
		           offsetof(struct user, u_debugreg[bpno]),
		           addr))
			exit2(1);

		/*
		 * set parameters for when the breakpoint should be triggered.
		 */
		if (ptrace(PTRACE_POKEUSER,
		           parent,
		           offsetof(struct user, u_debugreg[7]),
		           enable_breakwrite | enable_breakpoint))
			exit2(1);

		if (ptrace(PTRACE_DETACH, parent, NULL, NULL))
			exit2(1);

		exit2(0);
	}

	waitpid(child, &child_status, 0);

	signal(SIGTRAP, handler);

	if (WIFEXITED(child_status) && !WEXITSTATUS(child_status)) {
		return true;
	}

	assert(0 && "Failed to set breakpoint");
	return false;
}

[[gnu::no_instrument_function]]
void disablebp(int bpno) {
	breakpoint(NULL, bpno, NULL);
}

static uint64_t EvalRegOperand(const mcontext_t *mc, x86_reg reg) {
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

static uint64_t EvalMemOperand(const mcontext_t *mc, const x86_op_mem *mem) {
    uint64_t offset = mem->disp;
    uint64_t base = EvalRegOperand(mc, mem->base);
    uint64_t index = EvalRegOperand(mc, mem->index);
    return base + (index * mem->scale) + offset;
}

void kmtest::Machine::installSigSegv() {
    struct sigaction segv {
        .sa_sigaction = [](int, siginfo_t *, void *context) {
            ucontext_t *ucontext = reinterpret_cast<ucontext_t *>(context);
            mcontext_t *mcontext = &ucontext->uc_mcontext;
            gMachine->sigsegv(mcontext);
        },
        .sa_flags = SA_SIGINFO,
    };

    sigaction(SIGSEGV, &segv, nullptr);
}

void kmtest::Machine::rdmsr(mcontext_t *mcontext, cs_insn *insn) {
    uint32_t msr = mcontext->gregs[REG_RCX] & 0xFFFFFFFF;

    if (auto device = mMsrDevices.find(msr); device != mMsrDevices.end()) {
        auto *dev = device->second;
        uint64_t value = dev->rdmsr(msr);
        mcontext->gregs[REG_RAX] = value & 0xFFFFFFFF;
        mcontext->gregs[REG_RDX] = (value >> 32) & 0xFFFFFFFF;
        mcontext->gregs[REG_RIP] += insn[0].size;
        return;
    }

    KmDebugMessageUnlocked("unknown rdmsr: ", km::Hex(msr), "\n");
    exit(1);
}

void kmtest::Machine::wrmsr(mcontext_t *mcontext, cs_insn *insn) {
    uint32_t msr = mcontext->gregs[REG_RCX] & 0xFFFFFFFF;
    uint32_t low = mcontext->gregs[REG_RAX] & 0xFFFFFFFF;
    uint32_t high = mcontext->gregs[REG_RDX] & 0xFFFFFFFF;

    uint64_t value = low | ((uint64_t)high << 32);

    if (auto device = mMsrDevices.find(msr); device != mMsrDevices.end()) {
        auto *dev = device->second;
        dev->wrmsr(msr, value);
        mcontext->gregs[REG_RIP] += insn[0].size;
        return;
    }

    KmDebugMessageUnlocked("unknown wrmsr: ", km::Hex(msr), "\n");
    exit(1);
}

void kmtest::Machine::mmio(mcontext_t *mcontext, cs_insn *insn) {
    cs_detail *detail = insn->detail;
    cs_x86 *x86 = &detail->x86;
    cs_x86_op *op = x86->operands;

    //
    // This doesnt cover all possible memory operands for x86, "luckily" most hardware
    // shits itself if you do anything other than `mov [mem], reg` or `mov reg, [mem]`
    // on its mmio area so all our cases should be covered by this.
    //
    for (uint8_t i = 0; i < x86->op_count; i++) {
        if (op[i].type == X86_OP_MEM) {
            //
            // See if we have a memory operand that is in an mmio region
            //
            uint64_t address = EvalMemOperand(mcontext, &op[i].mem);

            auto& region = gMachine->mMmioRegions[{(void*)address}];
            auto& [mmio, host, guest, size] = region;
            if (mmio == nullptr) {
                //
                // If not then we can skip this operand... maybe this is a memcpy
                // between two mmio regions or something. (oh god)
                //
                continue;
            }

            mprotect(guest, size, PROT_READ | PROT_WRITE);

            try {
                mCurrentMmioRegion = &region;
                gMmioOffset = (address - (uintptr_t)guest);

                //
                // this is a write
                //
                if (op[i].access == CS_AC_WRITE) {
                    gMmioAccess = eAccessWrite;
                    mmio->writeBegin(gMmioOffset);
                    break;
                } else if (op[i].access == CS_AC_READ) {
                    gMmioAccess = eAccessRead;
                    mmio->readBegin(gMmioOffset);
                    break;
                }
            } catch (...) {
                mCurrentMmioRegion = nullptr;
                gMmioAccess = 0;
                int err = mprotect(mmio, size, PROT_NONE);
                if (err) {
                    perror("mprotect");
                    exit(1);
                }
            }
        }
    }

    void *pc = (void*)(mcontext->gregs[REG_RIP] + insn->size);

    // install a breakpoint on the next instruction
    breakpoint((void*)pc, 0, [](int) {
        auto& [mmio, host, guest, size] = *mCurrentMmioRegion;

        if (gMmioAccess == eAccessWrite) {
            mmio->writeEnd(gMmioOffset);
        } else if (gMmioAccess == eAccessRead) {
            mmio->readEnd(gMmioOffset);
        }

        int err = mprotect(guest, size, PROT_NONE);
        if (err) {
            perror("mprotect");
            exit(1);
        }
        disablebp(0);
    });
}

void kmtest::Machine::sigsegv(mcontext_t *mcontext) {
    void *rip = (void*)mcontext->gregs[REG_RIP];

    cs_insn *insn = cs_malloc(gCapstone);
    if (!insn) {
        perror("cs_malloc");
        exit(1);
    }

    defer { cs_free(insn, 1); };
    size_t size = 15;
    uint64_t pc = (uint64_t)rip;
    uint8_t code[15]{};
    VALGRIND_MAKE_MEM_DEFINED(rip, 15); // this is coming from a mapped area of text
    memcpy(code, rip, size);
    const uint8_t *ptr = code;
    bool ok = cs_disasm_iter(gCapstone, &ptr, &size, &pc, insn);
    if (!ok) {
        perror("cs_disasm_iter");
        exit(1);
    }

    dprintf(1, "fault: %s %s\n", insn[0].mnemonic, insn[0].op_str);

    if (insn[0].id == X86_INS_RDMSR) {
        rdmsr(mcontext, insn);
    } else if (insn[0].id == X86_INS_WRMSR) {
        wrmsr(mcontext, insn);
    } else {
        mmio(mcontext, insn);
    }
}

void kmtest::Machine::protectMmioRegions() {
    for (auto &[key, _] : mMmioRegions) {
        int err = mprotect(key.begin, 0x1000, PROT_NONE);
        if (err) {
            perror("mprotect");
            exit(1);
        }
    }
}

static constexpr size_t kMachineMemorySize = sm::gigabytes(1).bytes();

kmtest::Machine::Machine() {
    initAddressSpace();
    mprotect(mHostAddressSpace, kMachineMemorySize, PROT_READ | PROT_WRITE);
    mHostAllocator.deallocate(mHostAddressSpace, kMachineMemorySize / 0x1000);
}

void kmtest::Machine::initAddressSpace() {
    mAddressSpaceFd = memfd_create("address-space", 0);
    if (mAddressSpaceFd < 0) {
        perror("memfd_create");
        exit(1);
    }

    size_t size = kMachineMemorySize;
    if (ftruncate64(mAddressSpaceFd, size) < 0) {
        perror("ftruncate64");
        exit(1);
    }

    mHostAddressSpace = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, mAddressSpaceFd, 0);
    if (mHostAddressSpace == MAP_FAILED) {
        perror("mmap mHostAddressSpace");
        exit(1);
    }
}

void kmtest::Machine::setup() {
    cs_err err = cs_open(CS_ARCH_X86, CS_MODE_64, &gCapstone);
    if (err != CS_ERR_OK) {
        perror("cs_open");
        exit(1);
    }

    cs_option(gCapstone, CS_OPT_DETAIL, CS_OPT_ON);
    cs_option(gCapstone, CS_OPT_SKIPDATA, CS_OPT_ON);

    installSigSegv();
}

void kmtest::Machine::finalize() {
    cs_close(&gCapstone);
    delete gMachine;
}

void kmtest::Machine::reset(kmtest::Machine *machine) {
    delete gMachine;
    gMachine = machine;
    gMachine->protectMmioRegions();
}

kmtest::Machine *kmtest::Machine::instance() {
    return gMachine;
}

kmtest::Machine::~Machine() {
    close(mAddressSpaceFd);
}
