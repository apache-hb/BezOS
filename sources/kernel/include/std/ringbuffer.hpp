#pragma once

#include "common/util/util.hpp"
#include "std/std.hpp"

#include <emmintrin.h>
#include <memory>
#include <atomic>

#include <stddef.h>

#include <bezos/status.h>

namespace sm {
    /// @brief A fixed size, multi-producer, single-consumer reentrant atomic ringbuffer.
    /// @cite FreeBSDRingBuffer
    template<typename T>
    class AtomicRingQueue {
        static_assert(std::is_nothrow_move_constructible_v<T>
                   && std::is_nothrow_move_assignable_v<T>
                   && std::is_nothrow_destructible_v<T>
                   && std::is_nothrow_default_constructible_v<T>);

        std::unique_ptr<T[]> mStorage;
        uint32_t mCapacity{0};
        std::atomic<uint32_t> mProducerHead{0};
        std::atomic<uint32_t> mConsumerTail{0};
        std::atomic<uint32_t> mProducerTail{0};
        std::atomic<uint32_t> mConsumerHead{0};

    public:
        constexpr AtomicRingQueue() noexcept = default;
        UTIL_NOCOPY(AtomicRingQueue);

        constexpr AtomicRingQueue(AtomicRingQueue&& other) noexcept
            : mStorage(std::move(other.mStorage))
            , mCapacity(other.mCapacity)
            , mProducerHead(other.mProducerHead.load())
            , mConsumerTail(other.mConsumerTail.load())
            , mProducerTail(other.mProducerTail.load())
            , mConsumerHead(other.mConsumerHead.load())
        {
            other.mCapacity = 0;
        }

        /// @brief Try to push a value onto the queue.
        ///
        /// Attempts to push a value onto the queue. If the queue is full, the value is not pushed and false is returned.
        /// If the value is successfully pushed then @p value is moved from, otherwise it is left unchanged.
        ///
        /// @param value The value to push.
        ///
        /// @return true if the value was pushed, false if the queue was full.
        bool tryPush(T& value) noexcept [[clang::reentrant, clang::nonblocking]] {
            uint32_t producerHead;
            uint32_t producerTail;
            uint32_t producerTailNext;
            uint32_t producerNext;
            uint32_t consumerTail;

            do {
                producerHead = mProducerHead.load();
                consumerTail = mConsumerTail.load();

                producerNext = (producerHead + 1) % mCapacity;
                if (producerNext == consumerTail) {
                    return false; // Queue is full
                }
            } while (!mProducerHead.compare_exchange_strong(producerHead, producerNext));

            mStorage[producerHead] = std::move(value);

            do {
                producerTail = mProducerTail.load();
                producerTailNext = (producerTail + 1) % mCapacity;
            } while (!mProducerTail.compare_exchange_strong(producerTail, producerTailNext));

            return true;
        }

        /// @brief Try to pop a value from the queue.
        ////
        /// Attempts to pop a value from the queue. If the queue is empty, false is returned and @p value is left unchanged.
        /// If a value is successfully popped, it is moved into @p value.
        ///
        /// @param value The value to pop into.
        ///
        /// @return true if a value was popped, false if the queue was empty.
        bool tryPop(T& value) noexcept [[clang::reentrant, clang::nonblocking]] {
            uint32_t consumerHead = mConsumerHead.load();
            uint32_t producerTail = mProducerTail.load();

            uint32_t consumerNext = (consumerHead + 1) % mCapacity;

            if (consumerHead == producerTail) {
                return false; // Queue is empty
            }

            mConsumerHead.store(consumerNext);
            value = std::move(mStorage[consumerHead]);
            mConsumerTail.store(consumerNext);
            return true;
        }

        /// @brief Get an estimate of the number of items in the queue.
        ///
        /// @warning As this is a lock-free structure the count will be immediately out of date.
        ///
        /// @return The number of items in the queue.
        uint32_t count() const noexcept [[clang::reentrant, clang::nonblocking]] {
            uint32_t producerTail = mProducerTail.load();
            uint32_t consumerTail = mConsumerTail.load();
            return (mCapacity + producerTail - consumerTail) % mCapacity;
        }

        /// @brief Get the maximum capacity of the queue.
        ///
        /// @return The maximum number of items the queue can hold.
        uint32_t capacity() const noexcept [[clang::reentrant, clang::nonblocking]] {
            return mCapacity - 1;
        }

        /// @brief Check if the queue has been setup.
        ///
        /// @return true if the queue has been setup, false otherwise.
        bool isSetup() const noexcept [[clang::reentrant, clang::nonblocking]] {
            return mStorage != nullptr;
        }

        /// @brief Reset the queue to an empty state with the given storage and capacity.
        ///
        /// The storage must be at least capacity + 1 elements in size. The queue takes ownership of the storage.
        ///
        /// @param storage The storage to use for the queue.
        /// @param capacity The maximum number of elements the queue can hold.
        void reset(T *storage, uint32_t capacity) noexcept {
            mStorage.reset(storage);
            mCapacity = capacity + 1;
            mProducerHead.store(0);
            mProducerTail.store(0);
            mConsumerHead.store(0);
            mConsumerTail.store(0);
        }

        /// @brief Create a new queue with the given capacity.
        ///
        /// @param capacity The maximum number of elements the queue can hold.
        /// @param queue The created queue.
        ///
        /// @return The status of the operation.
        /// @retval OsStatusSuccess The queue was created successfully.
        /// @retval OsStatusInvalidInput The capacity was zero.
        /// @retval OsStatusOutOfMemory There was not enough memory to create the queue.
        [[nodiscard]]
        static OsStatus create(uint32_t capacity, AtomicRingQueue<T> *queue [[outparam]]) noexcept {
            if (capacity == 0) {
                return OsStatusInvalidInput;
            }

            if (T *storage = new (std::nothrow) T[capacity + 1]) {
                queue->reset(storage, capacity);
                return OsStatusSuccess;
            }

            return OsStatusOutOfMemory;
        }
    };
}
