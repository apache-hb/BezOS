#pragma once

#include "common/util/util.hpp"

#include <emmintrin.h>
#include <memory>
#include <atomic>

#include <stddef.h>

#include <bezos/status.h>

namespace sm {
    namespace detail {
        /// @param index The index to check
        /// @param start The start index of the circular buffer range.
        /// @param end The end index of the circular buffer range.
        constexpr bool isInCircular(uint32_t index, uint32_t start, uint32_t end) noexcept [[clang::reentrant, clang::nonblocking]] {
            return (start <= end) ? (index >= start && index < end) : (index >= start || index < end);
        }
    }

    /// @brief A fixed size, multi-producer, single-consumer reentrant atomic ringbuffer.
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

        bool isSetup() const noexcept [[clang::reentrant, clang::nonblocking]] {
            return mStorage != nullptr;
        }

        void reset(T *storage, uint32_t capacity) noexcept {
            mStorage.reset(storage);
            mCapacity = capacity + 1;
            mProducerHead.store(0);
            mProducerTail.store(0);
            mConsumerHead.store(0);
            mConsumerTail.store(0);
        }

        static OsStatus create(uint32_t capacity, AtomicRingQueue<T> *queue [[clang::noescape, gnu::nonnull]]) {
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
