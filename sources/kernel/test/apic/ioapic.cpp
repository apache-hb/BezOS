#include <gtest/gtest.h>

#include <latch>

#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <bits/syscall.h>
#include <unistd.h>

#include "util/defer.hpp"

#include <capstone.h>

#if __has_include(<valgrind/memcheck.h>)
#	include <valgrind/memcheck.h>
#else
#	define VALGRIND_MAKE_MEM_DEFINED(...)
#endif

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
bool breakpoint(void *addr, int bpno, void (*handler)(int)) {
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

void disablebp(int bpno) {
	breakpoint(NULL, bpno, NULL);
}

struct IoApicState {
	uint32_t regs[0x20 / sizeof(uint32_t)];
};

static void RemapMmio(void *mmio, int prot) {
	void *result = mmap(mmio, 0x1000, prot, MAP_FIXED | MAP_SHARED | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
	if (result == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}
}

struct IoApicTestState {
	IoApicState ioApicState{};

	IoApicState state{};

	enum { eNone, eRead, eWrite } next = eNone;

	uint32_t select = UINT32_MAX;

	void BeginRead(void *mmio) {
		RemapMmio(mmio, PROT_READ | PROT_WRITE);

		next = eRead;

		memcpy(mmio, &ioApicState, sizeof(IoApicState));
		RemapMmio(mmio, PROT_READ);
	}

	void EndRead(void *mmio) {
		next = eNone;

		RemapMmio(mmio, PROT_NONE);
	}

	void BeginWrite(void *mmio) {
		RemapMmio(mmio, PROT_READ | PROT_WRITE);

		next = eWrite;

		memcpy(mmio, &ioApicState, sizeof(IoApicState));
	}

	void EndWrite(void *mmio) {
		next = eNone;

		IoApicState newState{};
		memcpy(&newState, mmio, sizeof(IoApicState));

		if (newState.regs[0x0] != ioApicState.regs[0x0]) {
			// write to the window
			select = newState.regs[0x0];
		}

		RemapMmio(mmio, PROT_NONE);
	}

	void EndAccess(void *mmio) {
		switch (next) {
		case eRead:
			EndRead(mmio);
			break;
		case eWrite:
			EndWrite(mmio);
			break;
		default:
			break;
		}
	}
};

thread_local int a = 0;

class MmioTest : public ::testing::Test {
public:
    void SetUp() override {
		a = 5;

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

		fprintf(stderr, "%s:%u\n", __FILE__, __LINE__);

		cs_err err = cs_open(CS_ARCH_X86, CS_MODE_64, &gCapstone);
		ASSERT_EQ(err, CS_ERR_OK) << "Failed to open capstone: " << cs_strerror(err);

		cs_option(gCapstone, CS_OPT_DETAIL, CS_OPT_ON);
		cs_option(gCapstone, CS_OPT_SKIPDATA, CS_OPT_ON);

		fprintf(stderr, "%s:%u %zx\n", __FILE__, __LINE__, gCapstone);

		*TestState() = IoApicTestState{};
		RemapMmio(gMmioRegion, PROT_NONE);

		fprintf(stderr, "%s:%u\n", __FILE__, __LINE__);

        InstallSignals();

		fprintf(stderr, "%s:%u\n", __FILE__, __LINE__);

		long status = syscall(SYS_arch_prctl, 0x1003, &fsbase);
		if (status) {
			perror("arch_prctl");
			exit(1);
		}
		fprintf(stderr, "%s:%u %lx\n", __FILE__, __LINE__, fsbase);
    }

    void TearDown() override {
		cs_close(&gCapstone);
        munmap(gMmioRegion, 0x1000);
    }

	static inline uint64_t fsbase = 0;
    static inline void *gMmioRegion = nullptr;
	static inline csh gCapstone;
	static inline void *gCurrentRip = nullptr;

	static inline IoApicTestState *TestState() {
		return (IoApicTestState*)gMmioRegion;
	}

    void InstallSignals() {
        struct sigaction segv {
            .sa_sigaction = [](int, siginfo_t *, void *context) {
                ucontext_t *ucontext = reinterpret_cast<ucontext_t *>(context);
				mcontext_t *mcontext = &ucontext->uc_mcontext;
				void *rip = (void*)mcontext->gregs[REG_RIP];

				long status = syscall(SYS_arch_prctl, 0x1002, fsbase);
				if (status) {
					perror("arch_prctl");
					exit(1);
				}

				dprintf(2, "%s:%u %lx %zx\n", __FILE__, __LINE__, fsbase, gCapstone);

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

				dprintf(2, "%s:%u %p\n", __FILE__, __LINE__, rip);

				cs_detail *detail = insn[0].detail;
				cs_x86 *x86 = &detail->x86;
				cs_x86_op *op = x86->operands;
				for (uint8_t i = 0; i < x86->op_count; i++) {
					if (op[i].type == X86_OP_MEM) {
						// this is a write
						if (op[i].access == CS_AC_WRITE) {
							fprintf(stderr, "%s:%u\n", __FILE__, __LINE__);
							TestState()->BeginWrite(gMmioRegion);
							fprintf(stderr, "%s:%u\n", __FILE__, __LINE__);
							break;
						} else if (op[i].access == CS_AC_READ) {
							fprintf(stderr, "%s:%u\n", __FILE__, __LINE__);
							TestState()->BeginRead(gMmioRegion);
							fprintf(stderr, "%s:%u\n", __FILE__, __LINE__);
							break;
						}
					}
				}

				fprintf(stderr, "%s:%u\n", __FILE__, __LINE__);

				// install a breakpoint on the next instruction
				breakpoint((void*)pc, 0, [](int) {
					fprintf(stderr, "%s:%u\n", __FILE__, __LINE__);
					TestState()->EndAccess(gMmioRegion);
					fprintf(stderr, "%s:%u\n", __FILE__, __LINE__);
					disablebp(0);
				});

				// the user code actually accessed something else, panic
				if (TestState()->next == IoApicTestState::eNone) {
					write(2, "Invalid access\n", 15);
					exit(1);
				}
            },
            .sa_flags = SA_SIGINFO,
        };

        sigaction(SIGSEGV, &segv, nullptr);
    }
};

TEST_F(MmioTest, TestMmio) {
	((uint32_t*)gMmioRegion)[0x0] = 0x12345678;

	ASSERT_EQ(TestState()->select, 0x12345678);
}
