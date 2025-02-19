#include "log.hpp"

#include "std/spinlock.hpp"
#include "std/recursive_mutex.hpp"

#include <new>
#include <utility>

class IDebugLock {
public:
    virtual ~IDebugLock() = default;

    void operator delete(IDebugLock*, std::destroying_delete_t) {
        std::unreachable();
    }

    virtual void lock() = 0;
    virtual void unlock() = 0;
};

class NullLock final : public IDebugLock {
public:
    void lock() override { }
    void unlock() override { }
};

class SpinLockDebugLock final : public IDebugLock {
    stdx::SpinLock mLock;
public:
    void lock() override {
        mLock.lock();
    }

    void unlock() override {
        mLock.unlock();
    }
};

class RecursiveDebugLock final : public IDebugLock {
    stdx::RecursiveSpinLock mLock;

public:
    void lock() override {
        mLock.lock();
    }

    void unlock() override {
        mLock.unlock();
    }
};

constinit static NullLock gNullLock;
constinit static SpinLockDebugLock gSpinLock;
constinit static RecursiveDebugLock gRecursiveLock;

constinit static IDebugLock *gLogLock = &gNullLock;

// public interface

void km::SetDebugLogLock(DebugLogLockType type) {
    switch (type) {
    case DebugLogLockType::eNone:
        gLogLock = &gNullLock;
        break;
    case DebugLogLockType::eSpinLock:
        gLogLock = &gSpinLock;
        break;
    case DebugLogLockType::eRecursiveSpinLock:
        gLogLock = &gRecursiveLock;
        break;
    }
}

void km::LockDebugLog() {
    gLogLock->lock();
}

void km::UnlockDebugLog() {
    gLogLock->unlock();
}
