#include "pvtest/cpu.hpp"
#include "pvtest/pvtest.hpp"

#include <assert.h>
#include <unistd.h>
#include <signal.h>

#include <utility>

#include "common/util/defer.hpp"
#include "common/compiler/compiler.hpp"

static pthread_key_t gThreadKey;

[[noreturn, gnu::format(printf, 1, 2)]]
static void SignalAssert(const char *fmt, ...) {
    // TODO: vsnprintf is technically not signal safe, but i dont think glibc does anything not signal-unsafe in it...
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    write(STDERR_FILENO, buffer, len);
    abort();
}

void pv::CpuCore::setRegisterOperand(mcontext_t *mcontext, x86_reg reg, uint64_t value) noexcept {
    switch (reg) {
    case X86_REG_RAX: mcontext->gregs[REG_RAX] = value; break;
    case X86_REG_RBX: mcontext->gregs[REG_RBX] = value; break;
    case X86_REG_RCX: mcontext->gregs[REG_RCX] = value; break;
    case X86_REG_RDX: mcontext->gregs[REG_RDX] = value; break;
    case X86_REG_RSI: mcontext->gregs[REG_RSI] = value; break;
    case X86_REG_RDI: mcontext->gregs[REG_RDI] = value; break;
    case X86_REG_RBP: mcontext->gregs[REG_RBP] = value; break;
    case X86_REG_RSP: mcontext->gregs[REG_RSP] = value; break;
    case X86_REG_R8:  mcontext->gregs[REG_R8] = value; break;
    case X86_REG_R9:  mcontext->gregs[REG_R9] = value; break;
    case X86_REG_R10: mcontext->gregs[REG_R10] = value; break;
    case X86_REG_R11: mcontext->gregs[REG_R11] = value; break;
    case X86_REG_R12: mcontext->gregs[REG_R12] = value; break;
    case X86_REG_R13: mcontext->gregs[REG_R13] = value; break;
    case X86_REG_R14: mcontext->gregs[REG_R14] = value; break;
    case X86_REG_R15: mcontext->gregs[REG_R15] = value; break;
    default:
        SignalAssert("pv::CpuCore::setRegisterOperand() unknown register %d\n", std::to_underlying(reg));
    }
}

uint64_t pv::CpuCore::getRegisterOperand(mcontext_t *mcontext, x86_reg reg) noexcept {
    switch (reg) {
    case X86_REG_RAX: return mcontext->gregs[REG_RAX];
    case X86_REG_RBX: return mcontext->gregs[REG_RBX];
    case X86_REG_RCX: return mcontext->gregs[REG_RCX];
    case X86_REG_RDX: return mcontext->gregs[REG_RDX];
    case X86_REG_RSI: return mcontext->gregs[REG_RSI];
    case X86_REG_RDI: return mcontext->gregs[REG_RDI];
    case X86_REG_RBP: return mcontext->gregs[REG_RBP];
    case X86_REG_RSP: return mcontext->gregs[REG_RSP];
    case X86_REG_R8:  return mcontext->gregs[REG_R8];
    case X86_REG_R9:  return mcontext->gregs[REG_R9];
    case X86_REG_R10: return mcontext->gregs[REG_R10];
    case X86_REG_R11: return mcontext->gregs[REG_R11];
    case X86_REG_R12: return mcontext->gregs[REG_R12];
    case X86_REG_R13: return mcontext->gregs[REG_R13];
    case X86_REG_R14: return mcontext->gregs[REG_R14];
    case X86_REG_R15: return mcontext->gregs[REG_R15];
    case X86_REG_RIP: return mcontext->gregs[REG_RIP];
    default:
        SignalAssert("pv::CpuCore::getRegisterOperand() unknown register %d\n", std::to_underlying(reg));
    }
}

uint64_t pv::CpuCore::getMemoryOperand(mcontext_t *mcontext, const x86_op_mem *op) noexcept {
    uint64_t offset = op->disp;
    uint64_t base = (op->base == X86_REG_INVALID) ? 0 : getRegisterOperand(mcontext, op->base);
    uint64_t index = (op->index == X86_REG_INVALID) ? 0 : getRegisterOperand(mcontext, op->index);
    return base + (index * op->scale) + offset;
}

void pv::CpuCore::sigsegv(mcontext_t *mcontext) {
    SignalAssert("pv::CpuCore::sigsegv() SIGSEGV at %p\n", (void *)mcontext->gregs[REG_RIP]);
}

void pv::CpuCore::run() {
    sigset_t sigset;
    PV_POSIX_CHECK(sigemptyset(&sigset));
    PV_POSIX_CHECK(sigaddset(&sigset, SIGUSR1));
    PV_POSIX_CHECK(sigaddset(&sigset, SIGUSR2));

    siginfo_t siginfo;
    PV_POSIX_CHECK(sigwaitinfo(&sigset, &siginfo));
    switch (siginfo.si_signo) {
    case SIGUSR1: // destroy signal
        break;
    case SIGUSR2: // start signal
        break;

    default:
        SignalAssert("pv::CpuCore::start() unknown signal %d\n", siginfo.si_signo);
    }
}

void *pv::CpuCore::start(void *arg) {
    PV_POSIX_CHECK(pthread_setspecific(gThreadKey, arg));
    CpuCore *self = reinterpret_cast<CpuCore *>(arg);
    self->run();
    return nullptr;
}

pv::CpuCore::CpuCore() {
    pthread_attr_t attr;
    PV_POSIX_CHECK(pthread_attr_init(&attr));
    defer { pthread_attr_destroy(&attr); };

    sigset_t sigset;
    PV_POSIX_CHECK(sigfillset(&sigset));
    PV_POSIX_CHECK(sigdelset(&sigset, SIGSEGV));
    PV_POSIX_CHECK(sigdelset(&sigset, SIGUSR1));
    PV_POSIX_CHECK(sigdelset(&sigset, SIGUSR2));

    PV_POSIX_CHECK(pthread_attr_setsigmask_np(&attr, &sigset));

    PV_POSIX_CHECK(pthread_create(&mThread, &attr, &CpuCore::start, this));
}

pv::CpuCore::~CpuCore() {
    sigval val {
        .sival_ptr = this,
    };

    PV_POSIX_CHECK(pthread_sigqueue(mThread, SIGUSR1, val));

    PV_POSIX_CHECK(pthread_join(mThread, nullptr));
}

void pv::CpuCore::installSignals() {
    CLANG_DIAGNOSTIC_PUSH();
    CLANG_DIAGNOSTIC_IGNORE("-Wc99-designator");
    CLANG_DIAGNOSTIC_IGNORE("-Wmissing-designated-field-initializers");

    sigset_t sigset;
    PV_POSIX_CHECK(sigprocmask(SIG_BLOCK, nullptr, &sigset));
    PV_POSIX_CHECK(sigaddset(&sigset, SIGUSR1));
    PV_POSIX_CHECK(sigaddset(&sigset, SIGUSR2));
    PV_POSIX_CHECK(sigprocmask(SIG_SETMASK, &sigset, nullptr));

    PV_POSIX_CHECK(pthread_key_create(&gThreadKey, nullptr));

    struct sigaction sigsegv {
        .sa_sigaction = [](int, siginfo_t *, void *context) {
            CpuCore *cpu = reinterpret_cast<CpuCore *>(pthread_getspecific(gThreadKey));

            ucontext_t *ucontext = reinterpret_cast<ucontext_t *>(context);
            mcontext_t *mcontext = &ucontext->uc_mcontext;
            cpu->sigsegv(mcontext);
        },
        .sa_mask = sigset_t{},
        .sa_flags = SA_SIGINFO,
    };

    PV_POSIX_CHECK(sigaction(SIGSEGV, &sigsegv, nullptr));

    CLANG_DIAGNOSTIC_POP();
}
