#pragma once

#include "std/rcuptr.hpp"
#include "std/shared_spinlock.hpp"
#include "std/static_string.hpp"
#include "std/std.hpp"

#include <bezos/bezos.h>

namespace obj {
    class System;
    class Process;
    class Thread;

    using ObjectName = stdx::StaticString<OS_OBJECT_NAME_MAX>;

    class IObject : public sm::RcuIntrusivePtr<IObject> {
    public:
        virtual ~IObject() = default;

        virtual OsStatus getName(ObjectName *name [[outparam]]) const noexcept = 0;
        virtual OsStatus setName(const ObjectName& name) noexcept = 0;
    };

    class BaseObject : public IObject {
        static constexpr char kValidNameChars[] = "abcdefghijklmnopqrstuvwxyz"
                                                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                                   "0123456789"
                                                   "._-";

        static bool isValidObjectNameChar(char c) noexcept {
            for (char valid : kValidNameChars) {
                if (c == valid) {
                    return true;
                }
            }
            return false;
        }

        stdx::SharedSpinLock mLock;
        ObjectName mName GUARDED_BY(mLock);

    protected:
        void setNameUnlocked(const ObjectName& name) REQUIRES(mLock) {
            mName = name;
        }

        ObjectName getNameUnlocked() const REQUIRES_SHARED(mLock) {
            return mName;
        }

    public:
        BaseObject() = default;

        static bool verifyObjectName(const ObjectName& name) noexcept {
            if (name.isEmpty()) {
                return false;
            }

            for (char c : name) {
                if (!isValidObjectNameChar(c)) {
                    return false;
                }
            }

            return true;
        }

        OsStatus getName(ObjectName *name [[outparam]]) const noexcept final override {
            stdx::SharedLock guard(mLock);
            *name = getNameUnlocked();
            return OsStatusSuccess;
        }

        OsStatus setName(const ObjectName& name) noexcept final override {
            if (!verifyObjectName(name)) {
                return OsStatusInvalidInput;
            }

            stdx::UniqueLock guard(mLock);
            setNameUnlocked(name);
            return OsStatusSuccess;
        }
    };
}
