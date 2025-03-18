#include <gtest/gtest.h>

#include "isr/isr.hpp"

#include <latch>
#include <thread>
#include <signal.h>

struct IsrSignalInfo {
    km::IsrContext *context;
    std::latch *latch;
};

class IsrFaultTest : public ::testing::Test {
public:
    void SetUp() override {
        // reset the isr table for each test
        for (size_t i = 0; i < km::SharedIsrTable::kCount; i++) {
            gTable.install(i, km::DefaultIsrHandler);
        }
    }

    void TearDown() override {
        sigaction(SIGUSR1, nullptr, nullptr);
    }

    static inline km::SharedIsrTable gTable;

    void InitThread() {
        struct sigaction action {
            .sa_sigaction = [](int, siginfo_t *info, void *) {
                IsrSignalInfo *user = reinterpret_cast<IsrSignalInfo *>(info->si_value.sival_ptr);
                *user->context = gTable.invoke(user->context);
                write(1, "signal!\n", 8);
                user->latch->count_down();
            },
            .sa_flags = SA_SIGINFO,
        };

        sigaction(SIGUSR1, &action, nullptr);
    }

    void SendFault(km::IsrContext *context, pthread_t target) {
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGUSR1);
        pthread_sigmask(SIG_BLOCK, &mask, nullptr);

        std::latch latch{1};

        IsrSignalInfo info = {
            .context = context,
            .latch = &latch,
        };

        union sigval value;
        value.sival_ptr = &info;
        int status = pthread_sigqueue(target, SIGUSR1, value);
        ASSERT_EQ(status, 0) << "Failed to send signal: " << strerror(errno);

        latch.wait();
    }
};

TEST_F(IsrFaultTest, TestHandler) {
    InitThread();

    gTable.install(0, [](km::IsrContext *context) {
        context->error = 0xDEADBEEF;
        return *context;
    });

    km::IsrContext context {
        .error = 0,
        .rip = 0,
        .cs = 0,
        .rflags = 0,
        .rsp = 0,
        .ss = 0,
    };

    {
        pthread_t self = pthread_self();
        std::jthread thread([&, self] {
            SendFault(&context, self);
        });
    }

    EXPECT_EQ(context.error, 0xDEADBEEF);
}
