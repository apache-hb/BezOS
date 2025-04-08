#include "system/system.hpp"
#include "system/create.hpp"
#include "system_test.hpp"

class SystemTest : public SystemBaseTest { };

TEST_F(SystemTest, Construct) {
    km::SystemMemory memory = body.make();
    sys2::GlobalSchedule schedule(1, 1);
    sys2::System system(&schedule, &memory.pageTables(), &memory.pmmAllocator());
}

TEST_F(SystemTest, ConstructMultiple) {
    km::SystemMemory memory = body.make();
    sys2::GlobalSchedule schedule(128, 128);
    sys2::System system(&schedule, &memory.pageTables(), &memory.pmmAllocator());
}

TEST_F(SystemTest, CreateProcess) {
    km::SystemMemory memory = body.make(sm::megabytes(2).bytes());
    sys2::GlobalSchedule schedule(128, 128);
    sys2::System system(&schedule, &memory.pageTables(), &memory.pmmAllocator());

    sys2::ProcessCreateInfo createInfo {
        .name = "TEST",
        .supervisor = false,
    };

    std::unique_ptr<sys2::ProcessHandle> hProcess = nullptr;

    RecordMemoryUsage(memory);

    OsStatus status = sys2::CreateRootProcess(&system, createInfo, std::out_ptr(hProcess));
    ASSERT_EQ(status, OsStatusSuccess);

    sys2::ProcessDestroyInfo destroyInfo {
        .object = hProcess.get(),
        .exitCode = 0,
        .reason = eOsProcessExited,
    };

    sys2::InvokeContext invoke { &system, hProcess.get(), nullptr };
    status = sys2::SysDestroyProcess(&invoke, destroyInfo);
    ASSERT_EQ(status, OsStatusSuccess);

    CheckMemoryUsage(memory);
}

TEST_F(SystemTest, CreateProcessAsync) {
    km::SystemMemory memory = body.make(sm::megabytes(2).bytes());
    sys2::GlobalSchedule schedule(128, 128);
    sys2::System system(&schedule, &memory.pageTables(), &memory.pmmAllocator());

    RecordMemoryUsage(memory);

    std::vector<std::jthread> threads;
    static constexpr size_t kThreadCount = 8;
    static constexpr size_t kProcessCount = 128;
    std::barrier barrier(kThreadCount + 1);

    sys2::ProcessCreateInfo createInfo {
        .name = "TEST",
        .supervisor = false,
    };

    std::unique_ptr<sys2::ProcessHandle> hProcess = nullptr;
    sys2::ThreadHandle *hThread = nullptr;

    RecordMemoryUsage(memory);

    OsStatus status = sys2::CreateRootProcess(&system, createInfo, std::out_ptr(hProcess));
    ASSERT_EQ(status, OsStatusSuccess);

    sys2::ThreadCreateInfo threadCreateInfo {
        .name = "TEST",
        .process = hProcess.get(),
        .cpuState = {},
        .tlsAddress = 0,
        .state = eOsThreadRunning,
    };
    sys2::InvokeContext invoke { &system, hProcess.get(), nullptr };
    status = sys2::SysCreateThread(&invoke, threadCreateInfo, &hThread);
    ASSERT_EQ(status, OsStatusSuccess);

    threads.emplace_back([&](std::stop_token stop) {
        barrier.arrive_and_wait();

        while (!stop.stop_requested()) {
            system.rcuDomain().synchronize();
        }
    });

    for (size_t i = 0; i < kThreadCount; i++) {
        threads.emplace_back([&]() {
            barrier.arrive_and_wait();

            for (size_t j = 0; j < kProcessCount; j++) {
                sys2::ProcessHandle *hChild = nullptr;
                sys2::ProcessCreateInfo createInfo {
                    .name = "CHILD",
                    .process = hProcess.get(),
                    .supervisor = false,
                };

                sys2::InvokeContext invoke { &system, hProcess.get(), hThread };
                OsStatus status = sys2::SysCreateProcess(&invoke, createInfo, &hChild);
                if (status == OsStatusOutOfMemory) continue;
                ASSERT_EQ(status, OsStatusSuccess);

                sys2::ProcessDestroyInfo destroyInfo {
                    .object = hChild,
                    .exitCode = 0,
                    .reason = eOsProcessExited,
                };

                status = sys2::SysDestroyProcess(&invoke, destroyInfo);
                ASSERT_EQ(status, OsStatusSuccess);
            }
        });
    }

    threads.clear();

    sys2::ProcessDestroyInfo destroyInfo {
        .object = hProcess.get(),
        .exitCode = 0,
        .reason = eOsProcessExited,
    };

    status = sys2::SysDestroyProcess(&invoke, destroyInfo);
    ASSERT_EQ(status, OsStatusSuccess);

    CheckMemoryUsage(memory);
}

TEST_F(SystemTest, CreateChildProcess) {
    km::SystemMemory memory = body.make(sm::megabytes(2).bytes());
    sys2::GlobalSchedule schedule(128, 128);
    sys2::System system(&schedule, &memory.pageTables(), &memory.pmmAllocator());

    RecordMemoryUsage(memory);

    std::unique_ptr<sys2::ProcessHandle> hProcess = nullptr;
    sys2::ThreadHandle *hThread = nullptr;
    sys2::ProcessHandle *hChild = nullptr;

    {
        sys2::ProcessCreateInfo createInfo {
            .name = "TEST",
            .supervisor = false,
        };

        RecordMemoryUsage(memory);

        OsStatus status = sys2::CreateRootProcess(&system, createInfo, std::out_ptr(hProcess));
        ASSERT_EQ(status, OsStatusSuccess);
    }

    {

        sys2::ThreadCreateInfo threadCreateInfo {
            .name = "TEST",
            .process = hProcess.get(),
            .cpuState = {},
            .tlsAddress = 0,
            .state = eOsThreadRunning,
        };
        sys2::InvokeContext invoke { &system, hProcess.get(), nullptr };
        OsStatus status = sys2::SysCreateThread(&invoke, threadCreateInfo, &hThread);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    {
        sys2::ProcessCreateInfo createInfo {
            .name = "CHILD",
            .process = hProcess.get(),
            .supervisor = false,
        };
        sys2::InvokeContext invoke { &system, hProcess.get(), hThread };

        OsStatus status = sys2::SysCreateProcess(&invoke, createInfo, &hChild);
        ASSERT_EQ(status, OsStatusSuccess);

        ASSERT_NE(hChild, nullptr) << "Child process was not created";
        auto process = hProcess->getProcess();
        ASSERT_NE(process, nullptr) << "Parent process was not created";

        ASSERT_TRUE(process->getHandle(hChild->getHandle())) << "Parent process does not have child process";
    }

    sys2::ProcessDestroyInfo destroyInfo {
        .object = hProcess.get(),
        .exitCode = 0,
        .reason = eOsProcessExited,
    };

    sys2::InvokeContext invoke { &system, hProcess.get(), hThread };
    OsStatus status = sys2::SysDestroyProcess(&invoke, destroyInfo);
    ASSERT_EQ(status, OsStatusSuccess);

    CheckMemoryUsage(memory);
}

TEST_F(SystemTest, NestedChildProcess) {
    km::SystemMemory memory = body.make(sm::megabytes(8).bytes());
    sys2::GlobalSchedule schedule(128, 128);
    sys2::System system(&schedule, &memory.pageTables(), &memory.pmmAllocator());

    RecordMemoryUsage(memory);

    std::unique_ptr<sys2::ProcessHandle> hProcess = nullptr;

    {
        sys2::ProcessCreateInfo createInfo {
            .name = "TEST",
            .supervisor = false,
        };

        OsStatus status = sys2::CreateRootProcess(&system, createInfo, std::out_ptr(hProcess));
        ASSERT_EQ(status, OsStatusSuccess);
    }

    {
        sys2::ProcessHandle *hChild = hProcess.get();
        for (size_t i = 0; i < 8; i++) {
            sys2::ProcessCreateInfo createInfo {
                .name = "CHILD",
                .process = hChild,
                .supervisor = false,
            };

            DebugMemoryUsage(memory);

            sys2::InvokeContext invoke { &system, hChild, nullptr };
            OsStatus status = sys2::SysCreateProcess(&invoke, createInfo, &hChild);
            ASSERT_EQ(status, OsStatusSuccess);

            ASSERT_NE(hChild, nullptr) << "Child process was not created";
        }
    }

    sys2::ProcessDestroyInfo destroyInfo {
        .object = hProcess.get(),
        .exitCode = 0,
        .reason = eOsProcessExited,
    };

    sys2::InvokeContext invoke { &system, hProcess.get(), nullptr };
    OsStatus status = sys2::SysDestroyProcess(&invoke, destroyInfo);
    ASSERT_EQ(status, OsStatusSuccess);

    CheckMemoryUsage(memory);
}


TEST_F(SystemTest, OrphanThread) {
    km::SystemMemory memory = body.make(sm::megabytes(2).bytes());
    sys2::GlobalSchedule schedule(128, 128);
    sys2::System system(&schedule, &memory.pageTables(), &memory.pmmAllocator());

    sys2::ProcessCreateInfo processCreateInfo {
        .name = "TEST",
        .supervisor = false,
    };


    std::unique_ptr<sys2::ProcessHandle> hProcess = nullptr;
    sys2::ThreadHandle *hThread = nullptr;

    RecordMemoryUsage(memory);

    OsStatus status = sys2::CreateRootProcess(&system, processCreateInfo, std::out_ptr(hProcess));
    ASSERT_EQ(status, OsStatusSuccess);
    ASSERT_NE(hProcess, nullptr) << "Process was not created";

    sys2::ThreadCreateInfo threadCreateInfo {
        .name = "TEST",
        .process = hProcess.get(),
        .cpuState = {},
        .tlsAddress = 0,
        .state = eOsThreadRunning,
    };
    sys2::InvokeContext invoke { &system, hProcess.get(), nullptr };
    status = sys2::SysCreateThread(&invoke, threadCreateInfo, &hThread);
    ASSERT_EQ(status, OsStatusSuccess);

    ASSERT_NE(hThread, nullptr) << "Thread was not created";
    auto process = hProcess->getProcess();

    sys2::ProcessDestroyInfo destroyInfo {
        .object = hProcess.get(),
        .exitCode = 0,
        .reason = eOsProcessExited,
    };

    status = sys2::SysDestroyProcess(&invoke, destroyInfo);
    ASSERT_EQ(status, OsStatusSuccess);

    CheckMemoryUsage(memory);
}

TEST_F(SystemTest, CreateThread) {
    km::SystemMemory memory = body.make(sm::megabytes(2).bytes());
    sys2::GlobalSchedule schedule(128, 128);
    sys2::System system(&schedule, &memory.pageTables(), &memory.pmmAllocator());

    sys2::ProcessCreateInfo processCreateInfo {
        .name = "TEST",
        .supervisor = false,
    };

    std::unique_ptr<sys2::ProcessHandle> hProcess = nullptr;
    sys2::ThreadHandle *hThread = nullptr;

    auto before0 = GetMemoryState(memory);

    OsStatus status = sys2::CreateRootProcess(&system, processCreateInfo, std::out_ptr(hProcess));
    ASSERT_EQ(status, OsStatusSuccess);
    ASSERT_NE(hProcess, nullptr) << "Process was not created";

    auto before1 = GetMemoryState(memory);

    sys2::ThreadCreateInfo threadCreateInfo {
        .name = "TEST",
        .process = hProcess.get(),
        .cpuState = {},
        .tlsAddress = 0,
        .state = eOsThreadRunning,
    };
    sys2::InvokeContext invoke { &system, hProcess.get(), nullptr };

    status = sys2::SysCreateThread(&invoke, threadCreateInfo, &hThread);
    ASSERT_EQ(status, OsStatusSuccess);

    ASSERT_NE(hThread, nullptr) << "Thread was not created";
    auto process = hProcess->getProcess();

    sys2::ThreadDestroyInfo threadDestroyInfo {
        .object = hThread,
        .reason = eOsThreadOrphaned,
    };
    status = sys2::SysDestroyThread(&invoke, threadDestroyInfo);
    ASSERT_EQ(status, OsStatusSuccess);

    CheckMemoryState(memory, before1);

    sys2::ProcessDestroyInfo destroyInfo {
        .object = hProcess.get(),
        .exitCode = 0,
        .reason = eOsProcessExited,
    };

    status = sys2::SysDestroyProcess(&invoke, destroyInfo);
    ASSERT_EQ(status, OsStatusSuccess);

    CheckMemoryState(memory, before0);
}
