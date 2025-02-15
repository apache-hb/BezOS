#pragma once

#include "std/forward_list.hpp"
#include "std/shared_spinlock.hpp"
#include "std/spinlock.hpp"

#include <atomic>

namespace sm {
    template<typename T>
    class RcuDomain;

    template<typename T>
    class RcuGuard;

    template<typename T>
    class RcuDomain {
    public:

        using Guard = RcuGuard<T>;
        friend Guard;

        /// @brief A generation of the RCU domain.
        ///
        /// Each generation tracks the number of reader threads currently accessing it.
        /// As well as all the data that can be reclaimed once the last reader has finished.
        class RcuGeneration {
            friend RcuDomain;
            friend Guard;

            std::atomic<uint32_t> guard;
            AtomicForwardList<T*> retired;
        };

        UTIL_NOCOPY(RcuDomain);
        UTIL_NOMOVE(RcuDomain);

        constexpr RcuDomain() = default;

        constexpr ~RcuDomain() {
            destroy(mCurrent.load());
        }

        /// @brief Wait until all current readers have finished.
        ///
        /// @details This function will block until all readers that are currently
        ///          reading have finished. If new readers arrive after this function is
        ///          called, this function will not wait for them to finish.
        void synchronize() {
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

        /// @brief Mark an object as retired.
        ///
        /// Retiring an object means that it is no longer current and should be
        /// reclaimed when all readers have finished with it.
        ///
        /// @param object The object to retire.
        void retire(T *object) {
            RcuGeneration *generation = acquire();
            generation->retired.push(object);
            generation->guard -= 1;
        }

    private:
        std::atomic<RcuGeneration*> mCurrent = new RcuGeneration();
        stdx::SpinLock mCurrentMutex;

        void destroy(RcuGeneration *generation) {
            while (T *head = generation->retired.pop(nullptr)) {
                delete head;
            }

            delete generation;
        }

        RcuGeneration *acquire() {
            //
            // For now we take a lock when accessing the current head.
            // Ideally this would be totally lock free but the lock is held
            // for very short periods of time and infrequently.
            //
            stdx::LockGuard guard(mCurrentMutex);
            RcuGeneration *current = mCurrent.load();
            current->guard += 1;
            return current;
        }

        RcuGeneration *exchange(RcuGeneration *generation) {
            stdx::LockGuard guard(mCurrentMutex);
            return mCurrent.exchange(generation);
        }
    };

    template<typename T>
    class RcuGuard {
    public:
        using Domain = RcuDomain<T>;

        RcuGuard(Domain& domain) {
            //
            // Copy the generation into the guard, acquiring also increments the reader
            // count so this wont be deleted from under us.
            //
            mGeneration = domain.acquire();
        }

        ~RcuGuard() {
            mGeneration->unlock_shared();
        }

        UTIL_NOCOPY(RcuGuard);
        UTIL_NOMOVE(RcuGuard);

    private:
        using Generation = typename Domain::RcuGeneration;
        Generation *mGeneration = nullptr;
    };
}
