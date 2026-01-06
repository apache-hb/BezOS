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

#include <fmt/format.h>

#include <quill/Frontend.h>
#include <quill/Logger.h>
#include <quill/LogMacros.h>

namespace {
static constexpr size_t kPageSize = 0x1000;
static constexpr size_t kStackPages = 256;

struct alignas(kPageSize) StackPage {
    std::byte data[kPageSize];
};

struct SandboxArgs {
    pkg::Sandbox sandbox;
    uid_t euid;
    gid_t egid;
    std::function<void()>* callback;
};

int runSandboxInner(SandboxArgs *args) {
    auto sandbox = args->sandbox;

    if (sandbox.chroot.has_value()) {
        int err = chroot(sandbox.chroot.value().string().c_str());
        if (err == -1) {
            int eno = errno;
            throw std::runtime_error(fmt::format("chroot {} failed ({}: {})", sandbox.chroot.value().string(), eno, strerror(eno)));
        }
    }

    auto* callback = args->callback;
    (*callback)();
    return 0;
}

int runSandboxCallback(void *arg) {
    SandboxArgs *args = static_cast<SandboxArgs*>(arg);
    auto logger = quill::Frontend::create_or_get_logger("Sandbox", quill::Frontend::get_logger("root"));

    try {
        return runSandboxInner(args);
    } catch (const std::exception& e) {
        LOG_ERROR(logger, "Failed to execute sandbox: {}", e.what());
        return -1;
    }
}
}

void pkg::runCommandInSandbox(
    const Sandbox& sandbox,
    std::function<void()> run
) {
    std::unique_ptr<StackPage[]> stack = std::make_unique<StackPage[]>(kStackPages);
    void *stackTop = stack.get() + kStackPages;

    int flags = CLONE_FILES | CLONE_VM | CLONE_IO | CLONE_NEWPID | CLONE_NEWUSER | CLONE_NEWNS;

    if (!hasCapability(sandbox.capabilities, SandboxCapability::eNetworkAccess)) {
        flags |= CLONE_NEWNET;
    }

    uid_t euid = geteuid();
    gid_t egid = getegid();

    SandboxArgs args{sandbox, euid, egid, &run};
    int pid = clone(runSandboxCallback, stackTop, flags | SIGCHLD, &args);
    if (pid == -1) {
        int eno = errno;
        throw std::runtime_error(fmt::format("Failed to create sandboxed process ({}: {})", eno, strerror(eno)));
    }

    int status = 0;
    if (int err = waitpid(pid, &status, 0); err == -1) {
        int eno = errno;
        throw std::runtime_error(fmt::format("Failed to wait for sandboxed process ({}: {})", eno, strerror(eno)));
    }

    if (WEXITSTATUS(status) != 0) {
        throw std::runtime_error(fmt::format("Sandboxed process exited with error {}", WEXITSTATUS(status)));
    }
}
