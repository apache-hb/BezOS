#pragma once

#include <bezos/facility/process.h>
#include <bezos/facility/threads.h>

#include "std/shared_spinlock.hpp"
#include "std/spinlock.hpp"
#include "std/static_string.hpp"
#include "std/string.hpp"

namespace km {
    class SystemMemory;

    using ObjectName = stdx::StaticString<OS_OBJECT_NAME_MAX>;

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

    class BaseObject;

    struct HandleWait {
        BaseObject *object = nullptr;
        OsInstant timeout = OS_TIMEOUT_INSTANT;
    };

    class IObject {
    public:
        virtual ~IObject() = default;

        virtual ObjectName getName() = 0;
        virtual void setName(ObjectName name) = 0;
    };

    class BaseObject : public IObject {
    protected:
        stdx::SharedSpinLock mLock;

    private:
        OsHandle mId;
        ObjectName mName GUARDED_BY(mLock);

        uint32_t mStrongCount = 1;
        uint32_t mWeakCount = 1;

    public:
        virtual ~BaseObject() = default;

        BaseObject() = default;

        BaseObject(OsHandle id, stdx::String name)
            : mId(id)
            , mName(std::move(name))
        { }

        void initHeader(OsHandle id, OsHandleType type, stdx::String name);

        OsHandle publicId() const { return mId; }
        OsHandleType handleType() const { return OS_HANDLE_TYPE(mId); }
        OsHandle internalId() const { return OS_HANDLE_ID(mId); }

        ObjectName getName() final override;
        void setName(ObjectName name) final override;

        bool retainStrong();
        void retainWeak();

        bool releaseStrong();
        bool releaseWeak();
    };
}
