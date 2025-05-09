#include "std/rcu.hpp"
#include "panic.hpp"

#include <emmintrin.h>
#include <new>

sm::RcuDomain::~RcuDomain() noexcept {
    synchronize();
    synchronize();
}

void sm::RcuDomain::synchronize() noexcept [[clang::allocating, clang::blocking, clang::nonreentrant]] {
    //
    // Swap out the current generation with a new one and then
    // loop until all readers have finished with data in this generation.
    //
    RcuGeneration *generation = exchange();
    while (generation->mGuard > 0) {
        _mm_pause();
    }

    //
    // Once it is safe to do so we cleanup all the old state. exchange()
    // ensures that no new readers can be added to this generation.
    //
    generation->destroy();
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

void sm::RcuDomain::RcuGeneration::append(RcuObject *object) noexcept [[clang::reentrant, clang::nonallocating]] {
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

void sm::RcuDomain::RcuGeneration::destroy() noexcept [[clang::nonreentrant]] {
    KM_ASSERT(mGuard == 0);

    RcuObject *head = mHead.load();
    while (head != nullptr) {
        RcuObject *next = head->rcuNextObject.load();
        head->rcuRetireFn(head);
        head = next;
    }

    mHead.store(nullptr);
}

void sm::RcuDomain::append(RcuObject *object) noexcept [[clang::reentrant]] {
    sm::RcuGuard guard(*this);
    guard.append(object);
}

sm::RcuDomain::RcuGeneration *sm::RcuDomain::acquire() noexcept [[clang::nonblocking, clang::reentrant]] {
    // Protect the current generation from being swapped out while we are
    // acquiring it. We then update its reader count and release the guard.
    // This prevents the generation from having its data cleaned up before the guard
    // is acquired.
    uint32_t state = mState.fetch_add(1, std::memory_order_acquire);
    RcuGeneration *current = &mGenerations[(state & kCurrentGeneration) ? 1 : 0];

    current->mGuard.fetch_add(1, std::memory_order_acquire);

    uint32_t newState = mState.fetch_sub(1, std::memory_order_release);

    // The buffer should never be swapped out from under us.
    KM_ASSERT((newState & kCurrentGeneration) == (state & kCurrentGeneration));

    return current;
}

sm::RcuDomain::RcuGeneration *sm::RcuDomain::exchange() noexcept [[clang::nonreentrant]] {
    uint32_t initial = mState.load(std::memory_order_acquire) & kCurrentGeneration;
    uint32_t state = initial;

    while (!mState.compare_exchange_strong(state, state ^ kCurrentGeneration, std::memory_order_acquire)) {
        //
        // The top bit of mState encodes the current generation. We need to wait
        // until it is either the only bit set, or there is no bit set at all.
        //
        _mm_pause();

        state &= kCurrentGeneration;
    }

    return &mGenerations[initial ? 1 : 0];
}

sm::RcuGuard::RcuGuard(RcuDomain& domain) noexcept [[clang::reentrant]]
    : mGeneration(domain.acquire())
{
    //
    // Copy the generation into the guard, acquiring also increments the reader
    // count so this wont be deleted from under us.
    //
}

sm::RcuGuard::~RcuGuard() noexcept [[clang::reentrant]] {
    if (mGeneration) mGeneration->mGuard -= 1;
}

sm::RcuGuard::RcuGuard(RcuGuard&& other) noexcept [[clang::reentrant]]
    : mGeneration(std::exchange(other.mGeneration, nullptr))
{ }

void sm::RcuGuard::append(RcuObject *object) noexcept [[clang::reentrant]] {
    mGeneration->append(object);
}
