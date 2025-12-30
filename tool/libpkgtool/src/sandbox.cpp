#include "pkgtool/sandbox.hpp"

#include <memory>

#include <linux/sched.h>
#include <sched.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

namespace {
static constexpr size_t kPageSize = 0x1000;
static constexpr size_t kStackPages = 256;

struct alignas(kPageSize) StackPage {
    std::byte data[kPageSize];
};

struct SandboxArgs {
    pkg::SandboxCapability capabilities;
    std::function<void()>* callback;
};

int runSandboxCallback(void *arg) {
    SandboxArgs *args = static_cast<SandboxArgs*>(arg);

    if (!hasCapability(args->capabilities, pkg::SandboxCapability::eWritableSourceDir)) {
        // remount source directory as read-only
    }

    auto* callback = args->callback;
    (*callback)();
    return 0;
}
}

void pkg::runCommandInSandbox(
    SandboxCapability capabilities,
    std::function<void()> run
) {
    std::unique_ptr<StackPage[]> stack = std::make_unique<StackPage[]>(kStackPages);
    void *stackTop = stack.get() + kStackPages;

    int flags = CLONE_FILES | CLONE_VM | CLONE_IO;

    if (!hasCapability(capabilities, SandboxCapability::eNetworkAccess)) {
        flags |= CLONE_NEWNET;
    }

    if (!hasCapability(capabilities, SandboxCapability::eWritableSourceDir)) {
        flags |= CLONE_NEWNS;
    }

    SandboxArgs args{capabilities, &run};
    int pid = clone(runSandboxCallback, stackTop, flags | SIGCHLD, &args);
    if (pid == -1) {
        throw std::runtime_error("Failed to create sandboxed process");
    }

    int status = 0;
    if (int err = waitpid(pid, &status, 0); err == -1) {
        int eno = errno;
        throw std::runtime_error(std::format("Failed to wait for sandboxed process ({}: {})", eno, strerror(eno)));
    }

    if (status != 0) {
        throw std::runtime_error(std::format("Sandboxed process exited with error {}", status));
    }
}
