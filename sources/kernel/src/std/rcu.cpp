#include "std/rcu.hpp"
#include "panic.hpp"

#include <emmintrin.h>
#include <new>

//
// RcuDomain implementation
//

sm::RcuDomain::~RcuDomain() noexcept {
    while (synchronize() > 0) {
        // repeatedly synchronize until all memory has been reclaimed.
    }
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
    return generation->destroy();
}

OsStatus sm::RcuDomain::call(void *data, void(*fn)(void*)) [[clang::allocating, clang::nonreentrant]] {
    struct RcuCall : public sm::RcuObject {
        void *value;
        void(*call)(void*);
    };

    if (RcuCall *call = new (std::nothrow) RcuCall()) {
        RcuGuard guard(*this);

        call->value = data;
        call->call = fn;
        call->rcuRetireFn = [](void *ptr) {
            RcuCall *call = static_cast<RcuCall*>(ptr);
            call->call(call->value);
            delete call;
        };

        guard.append(call);

        return OsStatusSuccess;
    }

    return OsStatusOutOfMemory;
}

void sm::RcuDomain::enqueue(RcuObject *object, void(*fn)(void*)) noexcept [[clang::reentrant, clang::nonblocking, clang::nonallocating]] {
    object->rcuRetireFn = fn;
    append(object);
}

void sm::RcuDomain::append(RcuObject *object) noexcept [[clang::reentrant, clang::nonblocking, clang::nonallocating]] {
    sm::RcuGuard guard(*this);
    guard.append(object);
}

sm::detail::RcuGeneration *sm::RcuDomain::acquire() noexcept [[clang::reentrant, clang::nonblocking]] {
    // Protect the current generation from being swapped out while we are
    // acquiring it. We then update its reader count and release the guard.
    // This prevents the generation from having its data cleaned up before the guard
    // is acquired.
    uint32_t state = mState.fetch_add(1, std::memory_order_acquire);
    detail::RcuGeneration *current = &mGenerations[(state & kCurrentGeneration) ? 1 : 0];

    current->lock(std::memory_order_acquire);

    uint32_t newState = mState.fetch_sub(1, std::memory_order_release);

    // The buffer should never be swapped out from under us.
    KM_ASSERT((newState & kCurrentGeneration) == (state & kCurrentGeneration));

    return current;
}

sm::detail::RcuGeneration *sm::RcuDomain::exchange() noexcept [[clang::reentrant]] {
    uint32_t current = mState.load(std::memory_order_acquire) & kCurrentGeneration;
    uint32_t expected = current;

    while (!mState.compare_exchange_strong(expected, current ^ kCurrentGeneration, std::memory_order_acquire)) {
        //
        // The top bit of mState encodes the current generation. We need to wait
        // until it is either the only bit set, or there is no bit set at all.
        //
        _mm_pause();

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
    object->rcuNextObject = head;

    while (!mHead.compare_exchange_strong(head, object)) {
        //
        // If the head has changed, we need to update the next pointer.
        //
        object->rcuNextObject = head;
    }
}

size_t sm::detail::RcuGeneration::destroy() noexcept [[clang::nonreentrant]] {
    KM_ASSERT(mGuard == 0);

    size_t count = 0;
    RcuObject *head = mHead.load();
    while (head != nullptr) {
        RcuObject *next = head->rcuNextObject.load();
        head->rcuRetireFn(head);
        head = next;

        count += 1;
    }

    mHead.store(nullptr);

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
    : mGeneration(domain.acquire())
{
    //
    // Copy the generation into the guard, acquiring also increments the reader
    // count so this wont be deleted from under us.
    //
}

sm::RcuGuard::~RcuGuard() noexcept [[clang::reentrant, clang::nonblocking]] {
    detail::RcuGeneration *generation = std::exchange(mGeneration, nullptr);
    if (generation) generation->unlock(std::memory_order_release);
}

sm::RcuGuard::RcuGuard(RcuGuard&& other) noexcept [[clang::reentrant]]
    : mGeneration(std::exchange(other.mGeneration, nullptr))
{ }

void sm::RcuGuard::enqueue(RcuObject *object [[gnu::nonnull]], void(*fn [[gnu::nonnull]])(void*)) noexcept [[clang::reentrant, clang::nonblocking]] {
    object->rcuRetireFn = fn;
    append(object);
}

void sm::RcuGuard::append(RcuObject *object) noexcept [[clang::reentrant, clang::nonblocking]] {
    mGeneration->append(object);
}
