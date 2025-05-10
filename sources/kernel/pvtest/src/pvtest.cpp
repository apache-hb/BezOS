#include "pvtest/pvtest.hpp"

#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/ucontext.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/user.h>
#include <sys/wait.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

extern "C" void __gcov_reset(void);
extern "C" void __gcov_dump(void);

#define ENABLE_BREAKPOINT(x) (0x1 << (x * 2))
#define ENABLE_BREAK_EXEC(x) (pv::kBreakExec << (16 + (x * 4)))
#define ENABLE_BREAK_WRITE(x) (pv::kBreakWrite << (16 + (x * 4)))
#define ENABLE_BREAK_READWRITE(x) (pv::kBreakData << (16 + (x * 4)))

[[gnu::no_instrument_function]]
bool pv::AddHwBreak(void *address, int bpnumber, void (*callback)(int)) noexcept {
    pid_t child = 0;
	uint32_t enableBreak = ENABLE_BREAKPOINT(bpnumber);
	uint32_t enableBreakOnWrite = ENABLE_BREAK_EXEC(bpnumber);
	pid_t parent = getpid();
	int child_status = 0;

	if (!(child = fork()))
	{
		int parent_status = 0;
		if (ptrace(PTRACE_ATTACH, parent, nullptr, nullptr))
			ExitFork(1);

		while (!WIFSTOPPED(parent_status))
			waitpid(parent, &parent_status, 0);

		/*
		 * set the breakpoint address.
		 */
		if (ptrace(PTRACE_POKEUSER, parent, offsetof(struct user, u_debugreg[bpnumber]), address)) {
            ExitFork(1);
        }

		/*
		 * set parameters for when the breakpoint should be triggered.
		 */
		if (ptrace(PTRACE_POKEUSER, parent, offsetof(struct user, u_debugreg[7]), enableBreakOnWrite | enableBreak)) {
            ExitFork(1);
        }

		if (ptrace(PTRACE_DETACH, parent, NULL, NULL)) {
            ExitFork(1);
        }

		ExitFork(0);
	}

	waitpid(child, &child_status, 0);

	signal(SIGTRAP, callback);

	if (WIFEXITED(child_status) && !WEXITSTATUS(child_status)) {
		return true;
	}

	assert(0 && "Failed to set breakpoint");
	return false;
}

[[gnu::no_instrument_function]]
bool pv::RemoveHwBreak(int bpnumber) noexcept {
    return AddHwBreak(nullptr, bpnumber, nullptr);
}

void pv::ExitFork(int status) noexcept {
    __gcov_dump();
    _exit(status);
}
