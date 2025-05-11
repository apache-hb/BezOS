#pragma once

#include "common/util/util.hpp"

#include <emmintrin.h>
#include <memory>
#include <atomic>

#include <stddef.h>

#include <bezos/status.h>

namespace sm {
    /// @brief A fixed size, multi-producer, single-consumer atomic ringbuffer.
    /// @cite FreeBSDRingBuffer
    template<typename T>
    class AtomicRingQueue {
        std::unique_ptr<T[]> mStorage;
        uint32_t mCapacity{0};
        std::atomic<uint32_t> mProducerHead{0};
        std::atomic<uint32_t> mProducerTail{0};
        std::atomic<uint32_t> mConsumerHead{0};
        std::atomic<uint32_t> mConsumerTail{0};

    public:
        constexpr AtomicRingQueue() noexcept = default;
        UTIL_NOCOPY(AtomicRingQueue);
        UTIL_DEFAULT_MOVE(AtomicRingQueue);

        bool tryPush(T& value) noexcept [[clang::reentrant, clang::nonblocking]] {
            uint32_t producerHead;
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

            while (!mProducerTail.compare_exchange_strong(producerHead, producerNext)) {
                // Spin until the tail is updated
                _mm_pause();
            }

            return true;
        }

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

        uint32_t count() const noexcept [[clang::reentrant, clang::nonblocking]] {
            uint32_t producerTail = mProducerTail.load();
            uint32_t consumerTail = mConsumerTail.load();
            return (mCapacity + producerTail - consumerTail) % mCapacity;
        }

        uint32_t capacity() const noexcept [[clang::reentrant, clang::nonblocking]] {
            return mCapacity - 1;
        }

        static OsStatus create(uint32_t capacity, AtomicRingQueue<T> *queue [[clang::noescape, gnu::nonnull]]) {
            if (capacity == 0) {
                return OsStatusInvalidInput;
            }

            if (T *storage = new (std::nothrow) T[capacity + 1]) {
                queue->mStorage.reset(storage);
                queue->mCapacity = capacity + 1;
                queue->mProducerHead.store(0);
                queue->mProducerTail.store(0);
                queue->mConsumerHead.store(0);
                queue->mConsumerTail.store(0);
                return OsStatusSuccess;
            }

            return OsStatusOutOfMemory;
        }
    };
}
