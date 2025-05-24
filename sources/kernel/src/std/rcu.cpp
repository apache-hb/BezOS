#include "std/rcu.hpp"
#include "panic.hpp"

#include <emmintrin.h>
#include <new>

//
// RcuDomain implementation
//

sm::RcuDomain::~RcuDomain() noexcept {
    size_t count = 0;
    while (count < std::size(mGenerations) + 1) {
        // repeatedly synchronize until all memory has been reclaimed.
        if (synchronize() == 0) {
            count += 1;
        } else {
            count = 0;
        }
    }

    KM_ASSERT(mGenerations[0].mHead.load() == nullptr);
    KM_ASSERT(mGenerations[1].mHead.load() == nullptr);
}

size_t sm::RcuDomain::synchronize() noexcept [[clang::allocating, clang::blocking, clang::nonreentrant]] {
    //
    // Swap out the current generation with a new one and then
    // loop until all readers have finished with data in this generation.
    //
    detail::RcuGeneration *generation = exchange();
    while (generation->isLocked()) {
        _mm_pause();
    }

    //
    // Once it is safe to do so we cleanup all the old state. exchange()
    // ensures that no new readers can be added to this generation.
    //
    return generation->destroy(this);
}

OsStatus sm::RcuDomain::call(void *data, RetireCallback fn) [[clang::allocating, clang::nonreentrant]] {
    // TODO: extract out the call allocation so we only take a read lock if we allocate successfully.
    RcuGuard guard(*this);
    return guard.call(data, fn);
}

void sm::RcuDomain::enqueue(RcuObject *object, RetireCallback fn) noexcept [[clang::reentrant, clang::nonblocking, clang::nonallocating]] {
    object->rcuRetireFn = fn;
    append(object);
}

void sm::RcuDomain::append(RcuObject *object) noexcept [[clang::reentrant, clang::nonblocking, clang::nonallocating]] {
    sm::RcuGuard guard(*this);
    guard.append(object);
}

sm::detail::RcuGeneration *sm::RcuDomain::acquireReadLock() noexcept [[clang::reentrant, clang::nonblocking]] {
    // Protect the current generation from being swapped out while we are
    // acquiring it. We then update its reader count and release the guard.
    // This prevents the generation from having its data cleaned up before the guard
    // is acquired.
    uint32_t state = mState.fetch_add(1);
    detail::RcuGeneration *current = selectGeneration(state);

    current->lock();

    return current;
}

void sm::RcuDomain::releaseReadLock(detail::RcuGeneration *generation) noexcept [[clang::reentrant]] {
    KM_ASSERT(generation == selectGeneration(mState.load()));
    generation->unlock();
    mState.fetch_sub(1);
}

sm::detail::RcuGeneration *sm::RcuDomain::exchange() noexcept [[clang::reentrant]] {
    uint32_t current = mState.load() & kCurrentGeneration;
    uint32_t expected = current;

    while (!mState.compare_exchange_strong(expected, current ^ kCurrentGeneration)) {
        //
        // The top bit of mState encodes the current generation. We need to wait
        // until it is either the only bit set, or there is no bit set at all.
        //
        expected = current;
    }

    return &mGenerations[current ? 1 : 0];
}

//
// RcuGeneration implementation
//

void sm::detail::RcuGeneration::append(RcuObject *object) noexcept [[clang::reentrant, clang::nonblocking]] {
    KM_ASSERT(mGuard > 0);
    KM_ASSERT(object->rcuRetireFn != nullptr);

    RcuObject *head = mHead.load();
    do {
        object->rcuNextObject = head;
    } while (!mHead.compare_exchange_strong(head, object));
}

size_t sm::detail::RcuGeneration::destroy(RcuDomain *domain) noexcept [[clang::nonreentrant]] {
    KM_ASSERT(mGuard == 0);

    size_t count = 0;
    RcuObject *head = mHead.exchange(nullptr);
    while (head != nullptr) {
        RcuObject *next = head->rcuNextObject.load();
        head->rcuRetireFn(domain, head);
        head = next;

        count += 1;
    }

    return count;
}

void sm::detail::RcuGeneration::lock(std::memory_order order) noexcept [[clang::reentrant]] {
    mGuard.fetch_add(1, order);
}

void sm::detail::RcuGeneration::unlock(std::memory_order order) noexcept [[clang::reentrant]] {
    mGuard.fetch_sub(1, order);
}

bool sm::detail::RcuGeneration::isLocked(std::memory_order order) const noexcept [[clang::reentrant]] {
    return mGuard.load(order) > 0;
}

//
// RcuGuard implementation
//

sm::RcuGuard::RcuGuard(RcuDomain& domain) noexcept [[clang::reentrant, clang::nonblocking]]
    : mDomain(&domain)
    , mGeneration(domain.acquireReadLock())
{
    //
    // Copy the generation into the guard, acquiring also increments the reader
    // count so this wont be deleted from under us.
    //
}

sm::RcuGuard::~RcuGuard() noexcept [[clang::reentrant, clang::nonblocking]] {
    detail::RcuGeneration *generation = std::exchange(mGeneration, nullptr);
    if (generation) {
        mDomain->releaseReadLock(generation);
    }
}

sm::RcuGuard::RcuGuard(RcuGuard&& other) noexcept [[clang::reentrant]]
    : mGeneration(std::exchange(other.mGeneration, nullptr))
{ }

void sm::RcuGuard::enqueue(RcuObject *object, RetireCallback fn) noexcept [[clang::reentrant, clang::nonblocking]] {
    object->rcuRetireFn = fn;
    append(object);
}

OsStatus sm::RcuGuard::call(void *data, RetireCallback fn) noexcept [[clang::allocating]] {
    struct RcuCall : public sm::RcuObject {
        void *value;
        RetireCallback call;
    };

    if (RcuCall *call = new (std::nothrow) RcuCall()) {
        call->value = data;
        call->call = fn;
        call->rcuRetireFn = [](RcuDomain *domain, void *ptr) {
            RcuCall *call = static_cast<RcuCall*>(ptr);
            call->call(domain, call->value);
            delete call;
        };

        append(call);

        return OsStatusSuccess;
    }

    return OsStatusOutOfMemory;
}

void sm::RcuGuard::append(RcuObject *object) noexcept [[clang::reentrant, clang::nonblocking]] {
    mGeneration->append(object);
}
