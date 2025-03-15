#pragma once

#include <bezos/facility/process.h>
#include <bezos/facility/threads.h>

#include "std/spinlock.hpp"
#include "std/string.hpp"

namespace km {
    class SystemMemory;

    enum class ThreadId : OsThreadHandle { };
    enum class ProcessId : OsProcessHandle { };
    enum class MutexId : OsMutexHandle { };
    enum class NodeId : OsNodeHandle { };
    enum class DeviceId : OsDeviceHandle { };

    template<typename T>
    class IdAllocator {
        std::atomic<std::underlying_type_t<T>> mCurrent = OS_HANDLE_INVALID + 1;

    public:
        T allocate() {
            return T(mCurrent.fetch_add(1));
        }
    };

    class KernelObject;

    struct HandleWait {
        KernelObject *object = nullptr;
        OsInstant timeout = OS_TIMEOUT_INSTANT;
    };

    class KernelObject {
        OsHandle mId;
        stdx::String mName;
        stdx::SpinLock mMonitor;

        uint32_t mStrongCount = 1;
        uint32_t mWeakCount = 1;

    public:
        virtual ~KernelObject() = default;

        KernelObject() = default;

        KernelObject(OsHandle id, stdx::String name)
            : mId(id)
            , mName(std::move(name))
        { }

        void initHeader(OsHandle id, OsHandleType type, stdx::String name);

        OsHandle publicId() const { return mId; }
        OsHandleType handleType() const { return OS_HANDLE_TYPE(mId); }
        OsHandle internalId() const { return OS_HANDLE_ID(mId); }

        stdx::StringView name() const { return mName; }

        bool retainStrong();
        void retainWeak();

        bool releaseStrong();
        bool releaseWeak();
    };
}
