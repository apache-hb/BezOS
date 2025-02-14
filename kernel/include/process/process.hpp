#pragma once

#include "arch/arch.hpp"
#include "fs2/node.hpp"
#include "isr.hpp"
#include "memory/memory.hpp"
#include "std/shared_spinlock.hpp"
#include "std/string.hpp"

#include "absl/container/flat_hash_map.h"
#include "std/vector.hpp"

namespace km {
    enum class ThreadId : uint64_t { };
    enum class AddressSpaceId : uint64_t { };
    enum class ProcessId : uint64_t { };

    template<typename TKey, typename TValue, typename THash = std::hash<TKey>, typename TEqual = std::equal_to<TKey>, typename TAllocator = mem::GlobalAllocator<std::pair<const TKey, TValue>>>
    using FlatHashMap = absl::flat_hash_map<TKey, TValue, THash, TEqual, TAllocator>;

    template<typename T>
    class IdAllocator {
        T mCurrent = T();

    public:
        T allocate() {
            auto result = mCurrent;
            mCurrent = T(std::to_underlying(mCurrent) + 1);
            return result;
        }
    };

    struct Thread;
    struct AddressSpace;
    struct Process;
    class SystemObjects;

    struct Thread {
        ThreadId id;
        stdx::String name;
        Process *process;
        km::IsrContext state;
        std::unique_ptr<std::byte[]> stack;
    };

    struct AddressSpace {
        AddressSpaceId id;
        stdx::String name;
        km::AddressMapping mapping;
    };

    struct Process {
        ProcessId id;
        stdx::String name;
        km::Privilege privilege;
        x64::MachineState machine;
        stdx::Vector2<Thread*> threads;
        stdx::Vector2<AddressSpace*> memory;
        stdx::Vector2<std::unique_ptr<vfs2::IVfsNodeHandle>> files;
    };

    class SystemObjects {
        stdx::SharedSpinLock mLock;

        IdAllocator<ThreadId> mThreadIds;
        IdAllocator<AddressSpaceId> mAddressSpaceIds;
        IdAllocator<ProcessId> mProcessIds;

        FlatHashMap<ThreadId, std::unique_ptr<Thread>> mThreads;
        FlatHashMap<AddressSpaceId, std::unique_ptr<AddressSpace>> mAddressSpaces;
        FlatHashMap<ProcessId, std::unique_ptr<Process>> mProcesses;

    public:
        SystemObjects() = default;

        Thread *createThread(stdx::String name, Process *process);
        AddressSpace *createAddressSpace(stdx::String name, km::AddressMapping mapping);
        Process *createProcess(stdx::String name, km::Privilege privilege);

        Thread *getThread(ThreadId id);
        AddressSpace *getAddressSpace(AddressSpaceId id);
        Process *getProcess(ProcessId id);
    };

    struct ProcessLaunch {
        Process *process;
        Thread *main;
    };
}
