#include "std/rcu.hpp"

sm::RcuDomain::~RcuDomain() {
    destroy(mCurrent.load());
}

void sm::RcuDomain::synchronize() {
    //
    // Swap out the current generation with a new one and then
    // loop until all readers have finished with data in this generation.
    //
    RcuGeneration *current = exchange(new RcuGeneration());
    while (current->guard != 0) { }

    //
    // Once it is safe to do so we cleanup all the old state.
    //
    destroy(current);
}

void sm::RcuDomain::call(void *data, void(*fn)(void*)) {
    RcuCall call = { data, fn };
    RcuGeneration *generation = acquire();
    generation->retired.enqueue(call);
    generation->guard -= 1;
}

void sm::RcuDomain::destroy(RcuGeneration *generation) {
    RcuCall head{};
    while (generation->retired.try_dequeue(head)) {
        head.call(head.value);
    }

    delete generation;
}

sm::RcuDomain::RcuGeneration *sm::RcuDomain::acquire() {
    //
    // For now we take a lock when accessing the current head.
    // Ideally this would be totally lock free but the lock is held
    // for very short periods of time and infrequently.
    //
    stdx::SharedLock guard(mCurrentMutex);
    RcuGeneration *current = mCurrent.load();
    current->guard += 1;
    return current;
}

sm::RcuDomain::RcuGeneration *sm::RcuDomain::exchange(RcuGeneration *generation) {
    stdx::UniqueLock guard(mCurrentMutex);
    return mCurrent.exchange(generation);
}

sm::RcuGuard::RcuGuard(RcuDomain& domain)
    : mGeneration(domain.acquire())
{
    //
    // Copy the generation into the guard, acquiring also increments the reader
    // count so this wont be deleted from under us.
    //
}

sm::RcuGuard::~RcuGuard() {
    if (mGeneration) mGeneration->guard -= 1;
}

sm::RcuGuard::RcuGuard(RcuGuard&& other)
    : mGeneration(std::exchange(other.mGeneration, nullptr))
{ }
