#include <gtest/gtest.h>

#include <emmintrin.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <bits/syscall.h>
#include <unistd.h>

#include "common/util/defer.hpp"


#include <capstone.h>

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

struct IoApicState {
	uint32_t regs[0x20 / sizeof(uint32_t)];
};

static void RemapMmio(void *mmio, int prot) {
	int result = mprotect(mmio, 0x1000, prot);
	if (result) {
		perror("mprotect");
		exit(1);
	}
}

struct IoApicTestState {
	IoApicState ioApicState{};

	IoApicState state{};

	enum { eNone, eRead, eWrite } next = eNone;

	uint32_t select = UINT32_MAX;

	void BeginRead() {
		RemapMmio(this, PROT_READ | PROT_WRITE);

		next = eRead;

		state = ioApicState;
		RemapMmio(this, PROT_READ);
	}

	void EndRead() {
		next = eNone;

		RemapMmio(this, PROT_NONE);
	}

	void BeginWrite() {
		RemapMmio(this, PROT_READ | PROT_WRITE);

		next = eWrite;

		state = ioApicState;
	}

	void EndWrite() {
		next = eNone;

		if (state.regs[0x0] != ioApicState.regs[0x0]) {
			// write to the window
			select = ioApicState.regs[0x0];
		}

		RemapMmio(this, PROT_NONE);
	}

	void EndAccess() {
		switch (next) {
		case eRead:
			EndRead();
			break;
		case eWrite:
			EndWrite();
			break;
		default:
			break;
		}
	}
};

class MmioTest : public ::testing::Test {
public:
    void SetUp() override {
		prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY);

        // apic mmio region is always in the first 4gb
        // mapped as prot none to catch any access
        // in the sigsegv handler we check if the access is a read or write
        // if it is a write then we map the page and set a breakpoint on the instruction afterwards
        // we then read the written value and apply it to the state machine, then unmap the page again
        // to catch the next write
        // if its a read then we we write the value from the state machine.
        // then we map the page and set a breakpoint on the instruction afterwards
        // we then unmap the page in the sigtrap handler
        gMmioRegion = mmap(nullptr, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
		if (gMmioRegion == MAP_FAILED) {
			perror("mmap");
			exit(1);
		}

		cs_err err = cs_open(CS_ARCH_X86, CS_MODE_64, &gCapstone);
		ASSERT_EQ(err, CS_ERR_OK) << "Failed to open capstone: " << cs_strerror(err);

		cs_option(gCapstone, CS_OPT_DETAIL, CS_OPT_ON);
		cs_option(gCapstone, CS_OPT_SKIPDATA, CS_OPT_ON);

		*TestState() = IoApicTestState{};
		RemapMmio(gMmioRegion, PROT_NONE);

        InstallSignals();
    }

    void TearDown() override {
		cs_close(&gCapstone);
        munmap(gMmioRegion, 0x1000);
    }

    static inline void *gMmioRegion = nullptr;
	static inline csh gCapstone;

	static inline IoApicTestState *TestState() {
		return (IoApicTestState*)gMmioRegion;
	}

    void InstallSignals() {
        struct sigaction segv {
            .sa_sigaction = [](int, siginfo_t *, void *context) {
                ucontext_t *ucontext = reinterpret_cast<ucontext_t *>(context);
				mcontext_t *mcontext = &ucontext->uc_mcontext;
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

				cs_detail *detail = insn[0].detail;
				cs_x86 *x86 = &detail->x86;
				cs_x86_op *op = x86->operands;
				for (uint8_t i = 0; i < x86->op_count; i++) {
					if (op[i].type == X86_OP_MEM) {
						// this is a write
						if (op[i].access == CS_AC_WRITE) {
							TestState()->BeginWrite();
							break;
						} else if (op[i].access == CS_AC_READ) {
							TestState()->BeginRead();
							break;
						}
					}
				}

				// install a breakpoint on the next instruction
				breakpoint((void*)pc, 0, [](int) {
					TestState()->EndAccess();
					disablebp(0);
				});
            },
            .sa_flags = SA_SIGINFO,
        };

        sigaction(SIGSEGV, &segv, nullptr);
    }
};

TEST_F(MmioTest, TestMmio) {
	auto *testState = TestState();
	testState->ioApicState.regs[0x0] = 0x12345678;

	RemapMmio(gMmioRegion, PROT_READ | PROT_WRITE);

	ASSERT_EQ(testState->select, 0x12345678);
}
