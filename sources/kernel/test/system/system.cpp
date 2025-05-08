#include "system/system.hpp"
#include "fs/vfs.hpp"
#include "system/create.hpp"
#include "system_test.hpp"

class SystemTest : public SystemBaseTest { };

TEST_F(SystemTest, Construct) {
    km::SystemMemory memory = body.make();
    vfs::VfsRoot vfs;
    sys::System system;
    OsStatus status = sys::System::create(&vfs, &memory.pageTables(), &memory.pmmAllocator(), &system);
    ASSERT_EQ(status, OsStatusSuccess);
}

TEST_F(SystemTest, CreateProcess) {
    km::SystemMemory memory = body.make(sm::megabytes(2).bytes());
    vfs::VfsRoot vfs;
    sys::System system;
    OsStatus status = sys::System::create(&vfs, &memory.pageTables(), &memory.pmmAllocator(), &system);
    ASSERT_EQ(status, OsStatusSuccess);

    sys::ProcessCreateInfo createInfo {
        .name = "TEST",
        .supervisor = false,
    };

    std::unique_ptr<sys::ProcessHandle> hProcess = nullptr;

    RecordMemoryUsage(memory);

    status = sys::SysCreateRootProcess(&system, createInfo, std::out_ptr(hProcess));
    ASSERT_EQ(status, OsStatusSuccess);

    sys::InvokeContext invoke { &system, hProcess->getProcess() };
    status = sys::SysProcessDestroy(&invoke, hProcess.get(), 0, eOsProcessExited);
    ASSERT_EQ(status, OsStatusSuccess);

    CheckMemoryUsage(memory);
}

TEST_F(SystemTest, CreateProcessAsync) {
    km::SystemMemory memory = body.make(sm::megabytes(2).bytes());
    vfs::VfsRoot vfs;
    sys::System system;
    OsStatus status = sys::System::create(&vfs, &memory.pageTables(), &memory.pmmAllocator(), &system);
    ASSERT_EQ(status, OsStatusSuccess);

    for (size_t i = 0; i < 8; i++) {
        system.scheduler()->initCpuSchedule(km::CpuCoreId(i), 128);
    }

    RecordMemoryUsage(memory);

    std::vector<std::jthread> threads;
    static constexpr size_t kThreadCount = 8;
    static constexpr size_t kProcessCount = 128;
    std::barrier barrier(kThreadCount + 1);

    sys::ProcessCreateInfo createInfo {
        .name = "TEST",
        .supervisor = false,
    };

    std::unique_ptr<sys::ProcessHandle> hProcess = nullptr;
    OsThreadHandle hThread = OS_HANDLE_INVALID;

    RecordMemoryUsage(memory);

    status = sys::SysCreateRootProcess(&system, createInfo, std::out_ptr(hProcess));
    ASSERT_EQ(status, OsStatusSuccess);

    sys::ThreadCreateInfo threadCreateInfo {
        .name = "TEST",
        .cpuState = {},
        .tlsAddress = 0,
        .state = eOsThreadRunning,
    };
    sys::InvokeContext invoke { &system, hProcess->getProcess() };
    status = sys::SysThreadCreate(&invoke, threadCreateInfo, &hThread);
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
                OsProcessHandle hChild = OS_HANDLE_INVALID;
                OsProcessCreateInfo createInfo {
                    .Name = "CHILD",
                    .Flags = eOsProcessSupervisor,
                };

                sys::InvokeContext invoke { &system, hProcess->getProcess(), GetThread(hProcess->getProcess(), hThread) };
                OsStatus status = sys::SysProcessCreate(&invoke, createInfo, &hChild);
                if (status == OsStatusOutOfMemory) continue;
                ASSERT_EQ(status, OsStatusSuccess);
                ASSERT_NE(hChild, OS_HANDLE_INVALID) << "Child process was not created";

                status = sys::SysProcessDestroy(&invoke, hChild, 0, eOsProcessExited);
                ASSERT_EQ(status, OsStatusSuccess);
            }
        });
    }

    threads.clear();

    status = sys::SysProcessDestroy(&invoke, hProcess.get(), 0, eOsProcessExited);
    ASSERT_EQ(status, OsStatusSuccess);

    CheckMemoryUsage(memory);
}

TEST_F(SystemTest, CreateChildProcess) {
    km::SystemMemory memory = body.make(sm::megabytes(2).bytes());
    vfs::VfsRoot vfs;
    sys::System system;
    OsStatus status = sys::System::create(&vfs, &memory.pageTables(), &memory.pmmAllocator(), &system);
    ASSERT_EQ(status, OsStatusSuccess);

    for (size_t i = 0; i < 8; i++) {
        system.scheduler()->initCpuSchedule(km::CpuCoreId(i), 128);
    }
    RecordMemoryUsage(memory);

    std::unique_ptr<sys::ProcessHandle> hProcess = nullptr;
    OsThreadHandle hThread = OS_HANDLE_INVALID;
    OsProcessHandle hChild = OS_HANDLE_INVALID;

    {
        sys::ProcessCreateInfo createInfo {
            .name = "TEST",
            .supervisor = false,
        };

        RecordMemoryUsage(memory);

        OsStatus status = sys::SysCreateRootProcess(&system, createInfo, std::out_ptr(hProcess));
        ASSERT_EQ(status, OsStatusSuccess);
    }

    {

        sys::ThreadCreateInfo threadCreateInfo {
            .name = "TEST",
            .cpuState = {},
            .tlsAddress = 0,
            .state = eOsThreadRunning,
        };
        sys::InvokeContext invoke { &system, hProcess->getProcess() };
        OsStatus status = sys::SysThreadCreate(&invoke, threadCreateInfo, &hThread);
        ASSERT_EQ(status, OsStatusSuccess);
    }

    {
        OsProcessCreateInfo createInfo {
            .Name = "CHILD",
            .Flags = eOsProcessSupervisor,
        };
        sys::InvokeContext invoke { &system, hProcess->getProcess(), GetThread(hProcess->getProcess(), hThread) };

        OsStatus status = sys::SysProcessCreate(&invoke, createInfo, &hChild);
        ASSERT_EQ(status, OsStatusSuccess);

        ASSERT_NE(hChild, OS_HANDLE_INVALID) << "Child process was not created";
        auto process = hProcess->getProcess();
        ASSERT_NE(process, nullptr) << "Parent process was not created";

        ASSERT_TRUE(process->getHandle(hChild)) << "Parent process does not have child process";
    }

    sys::InvokeContext invoke { &system, hProcess->getProcess(), GetThread(hProcess->getProcess(), hThread) };
    status = sys::SysProcessDestroy(&invoke, hProcess.get(), 0, eOsProcessExited);
    ASSERT_EQ(status, OsStatusSuccess);

    CheckMemoryUsage(memory);
}

TEST_F(SystemTest, NestedChildProcess) {
    km::SystemMemory memory = body.make(sm::megabytes(8).bytes());
    vfs::VfsRoot vfs;
    sys::System system;
    OsStatus status = sys::System::create(&vfs, &memory.pageTables(), &memory.pmmAllocator(), &system);
    ASSERT_EQ(status, OsStatusSuccess);

    for (size_t i = 0; i < 8; i++) {
        system.scheduler()->initCpuSchedule(km::CpuCoreId(i), 128);
    }
    RecordMemoryUsage(memory);

    std::unique_ptr<sys::ProcessHandle> hProcess = nullptr;

    {
        sys::ProcessCreateInfo createInfo {
            .name = "TEST",
            .supervisor = false,
        };

        OsStatus status = sys::SysCreateRootProcess(&system, createInfo, std::out_ptr(hProcess));
        ASSERT_EQ(status, OsStatusSuccess);
    }

    {
        OsProcessHandle hChild = hProcess->getHandle();
        sm::RcuSharedPtr<sys::Process> pChild = hProcess->getProcess();
        for (size_t i = 0; i < 8; i++) {
            OsProcessCreateInfo createInfo {
                .Name = "CHILD",
            };

            sys::InvokeContext invoke { &system, pChild };
            OsStatus status = sys::SysProcessCreate(&invoke, createInfo, &hChild);
            ASSERT_EQ(status, OsStatusSuccess);

            ASSERT_NE(hChild, OS_HANDLE_INVALID) << "Child process was not created";

            sys::ProcessHandle *hChildHandle = nullptr;
            status = pChild->findHandle(hChild, &hChildHandle);
            ASSERT_EQ(status, OsStatusSuccess) << "Parent process does not have child process";
            ASSERT_NE(hChildHandle, nullptr) << "Parent process does not have child process";

            hChild = hChildHandle->getHandle();
            pChild = hChildHandle->getProcess();
        }
    }

    sys::InvokeContext invoke { &system, hProcess->getProcess() };
    status = sys::SysProcessDestroy(&invoke, hProcess.get(), 0, eOsProcessExited);
    ASSERT_EQ(status, OsStatusSuccess);

    CheckMemoryUsage(memory);
}

TEST_F(SystemTest, OrphanThread) {
    km::SystemMemory memory = body.make(sm::megabytes(2).bytes());
    vfs::VfsRoot vfs;
    sys::System system;
    OsStatus status = sys::System::create(&vfs, &memory.pageTables(), &memory.pmmAllocator(), &system);
    ASSERT_EQ(status, OsStatusSuccess);

    for (size_t i = 0; i < 8; i++) {
        system.scheduler()->initCpuSchedule(km::CpuCoreId(i), 128);
    }
    sys::ProcessCreateInfo processCreateInfo {
        .name = "TEST",
        .supervisor = false,
    };


    std::unique_ptr<sys::ProcessHandle> hProcess = nullptr;
    OsThreadHandle hThread = OS_HANDLE_INVALID;

    RecordMemoryUsage(memory);

    status = sys::SysCreateRootProcess(&system, processCreateInfo, std::out_ptr(hProcess));
    ASSERT_EQ(status, OsStatusSuccess);
    ASSERT_NE(hProcess, nullptr) << "Process was not created";

    sys::ThreadCreateInfo threadCreateInfo {
        .name = "TEST",
        .cpuState = {},
        .tlsAddress = 0,
        .state = eOsThreadRunning,
    };
    sys::InvokeContext invoke { &system, hProcess->getProcess() };
    status = sys::SysThreadCreate(&invoke, threadCreateInfo, &hThread);
    ASSERT_EQ(status, OsStatusSuccess);

    ASSERT_NE(hThread, OS_HANDLE_INVALID) << "Thread was not created";
    auto process = hProcess->getProcess();

    status = sys::SysProcessDestroy(&invoke, hProcess.get(), 0, eOsProcessExited);
    ASSERT_EQ(status, OsStatusSuccess);

    CheckMemoryUsage(memory);
}

TEST_F(SystemTest, CreateThread) {
    km::SystemMemory memory = body.make(sm::megabytes(2).bytes());
    vfs::VfsRoot vfs;
    sys::System system;
    OsStatus status = sys::System::create(&vfs, &memory.pageTables(), &memory.pmmAllocator(), &system);
    ASSERT_EQ(status, OsStatusSuccess);

    for (size_t i = 0; i < 8; i++) {
        system.scheduler()->initCpuSchedule(km::CpuCoreId(i), 128);
    }
    sys::ProcessCreateInfo processCreateInfo {
        .name = "TEST",
        .supervisor = false,
    };

    std::unique_ptr<sys::ProcessHandle> hProcess = nullptr;
    OsThreadHandle hThread = OS_HANDLE_INVALID;

    auto before0 = GetMemoryState(memory);

    status = sys::SysCreateRootProcess(&system, processCreateInfo, std::out_ptr(hProcess));
    ASSERT_EQ(status, OsStatusSuccess);
    ASSERT_NE(hProcess, nullptr) << "Process was not created";

    auto before1 = GetMemoryState(memory);

    sys::ThreadCreateInfo threadCreateInfo {
        .name = "TEST",
        .cpuState = {},
        .tlsAddress = 0,
        .state = eOsThreadRunning,
    };
    sys::InvokeContext invoke { &system, hProcess->getProcess() };

    status = sys::SysThreadCreate(&invoke, threadCreateInfo, &hThread);
    ASSERT_EQ(status, OsStatusSuccess);

    ASSERT_NE(hThread, OS_HANDLE_INVALID) << "Thread was not created";
    auto process = hProcess->getProcess();

    status = sys::SysThreadDestroy(&invoke, eOsThreadOrphaned, hThread);
    ASSERT_EQ(status, OsStatusSuccess);

    CheckMemoryState(memory, before1);

    status = sys::SysProcessDestroy(&invoke, hProcess.get(), 0, eOsProcessExited);
    ASSERT_EQ(status, OsStatusSuccess);

    CheckMemoryState(memory, before0);
}
