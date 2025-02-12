#pragma once

#include "arch/arch.hpp"
#include "fs2/node.hpp"
#include "isr.hpp"
#include "std/shared_spinlock.hpp"
#include "std/string.hpp"

#include "absl/container/flat_hash_map.h"

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
        x64::RegisterState state;
        x64::MachineState machine;
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
        stdx::Vector2<ThreadId> threads;
        stdx::Vector2<AddressSpaceId> memory;
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

        Thread *createThread(stdx::String name) {
            stdx::UniqueLock guard(mLock);

            ThreadId id = mThreadIds.allocate();
            std::unique_ptr<Thread> thread{new Thread{id, std::move(name)}};
            Thread *result = thread.get();
            mThreads.insert({id, std::move(thread)});
            return result;
        }

        AddressSpace *createAddressSpace(stdx::String name, km::AddressMapping mapping) {
            stdx::UniqueLock guard(mLock);

            AddressSpaceId id = mAddressSpaceIds.allocate();
            std::unique_ptr<AddressSpace> addressSpace{new AddressSpace{id, std::move(name), mapping}};
            AddressSpace *result = addressSpace.get();
            mAddressSpaces.insert({id, std::move(addressSpace)});
            return result;
        }

        Process *createProcess(stdx::String name, km::Privilege privilege) {
            stdx::UniqueLock guard(mLock);

            ProcessId id = mProcessIds.allocate();
            std::unique_ptr<Process> process{new Process{id, std::move(name), privilege}};
            Process *result = process.get();
            mProcesses.insert({id, std::move(process)});
            return result;
        }

        Thread *getThread(ThreadId id) {
            stdx::SharedLock guard(mLock);
            if (auto it = mThreads.find(id); it != mThreads.end()) {
                return it->second.get();
            }

            return nullptr;
        }

        AddressSpace *getAddressSpace(AddressSpaceId id);
        Process *getProcess(ProcessId id);
    };

    struct ProcessLaunch {
        ProcessId process;
        ThreadId main;
    };
}
