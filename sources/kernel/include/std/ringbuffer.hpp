#pragma once

#include "panic.hpp"
#include "std/std.hpp"

#include <cstdio>
#include <cstddef>
#include <memory>
#include <atomic>
#include <limits>

#include <bezos/status.h>

#include "std/atomic_bitset.hpp"

namespace sm {
    /**
     * @brief A fixed size, multi-producer, single-consumer reentrant atomic ringbuffer.
     *
     * This ring buffer is reentrant unlike existing implementations because it must be safe to produce into
     * in an interrupt handler as well as in a normal execution context. This complicates its design as it is
     * not possible to implement a reentrant ring buffer without the elements in the ringbuffer being atomic
     * themselves. This means that this ring buffer also contains an atomic bitmap allocator to store its
     * contents.
     *
     * @tparam T The type of elements stored in the ring buffer. Must be MoveAssignable and MoveConstructible.
     * @tparam Allocator The allocator type used to allocate and deallocate memory for the ring buffer.
     *
     * @cite FreeBSDRingBuffer FreeBSD ring_buf implementation
     * @cite WaitFreeMpScQueue waitfree-mpsc-queue
     */
    template<typename T, typename Allocator = std::allocator<T>>
    class AtomicRingQueue {
    public:
        using size_type = uint32_t;
        using value_type = T;
        using allocator_type = Allocator;

    public:
        using BitSetAllocator = std::allocator_traits<Allocator>::template rebind_alloc<std::atomic<uint64_t>>;
        using ElementAllocator = std::allocator_traits<Allocator>::template rebind_alloc<std::atomic<size_type>>;

        static constexpr size_type kNotFound = (std::numeric_limits<size_type>::max)();

        [[no_unique_address]] Allocator mAllocator{};

        // The storage for the ring buffer, only the range of [normalize(mHead), normalize(mTail)) is valid initialized, all other slots are uninitialized.
        T* mStorage{};

        std::atomic<uint64_t> *mUsedBits{};
        std::atomic<size_type> *mElements{};

        // The capacity of the ring buffer + 1.
        size_type mCapacity{};

        std::atomic<size_type> mCount{};

        std::atomic<size_type> mHead{};
        std::atomic<size_type> mTail{};

        void clear() noexcept {
            if (mStorage) {
                //
                // Destroy the remaining elements in the buffer
                //
                constexpr size_t kBitsPerElement = std::numeric_limits<uint64_t>::digits;

                for (size_t i = 0; i < (capacity() / kBitsPerElement); i++) {
                    uint64_t word = mUsedBits[i].load();
                    for (size_t bit = 0; bit < kBitsPerElement; bit++) {
                        size_t index = i * kBitsPerElement + bit;
                        if (index >= capacity()) {
                            break;
                        }

                        if (word & (uint64_t{1} << bit)) {
                            std::destroy_at(&mStorage[index]);
                        }
                    }
                }

                mAllocator.deallocate(mStorage, mCapacity);

                BitSetAllocator bitsetAllocator{mAllocator};
                bitsetAllocator.deallocate(mUsedBits, detail::requiredBitsetSize(capacity()));

                ElementAllocator elementAllocator{mAllocator};
                elementAllocator.deallocate(mElements, capacity());

                mStorage = nullptr;
                mCapacity = 0;
            }
        }

        size_type normalize(size_type index) noexcept [[clang::nonblocking, clang::reentrant]] {
            return index % capacity();
        }

        size_t allocateElement() noexcept [[clang::nonblocking, clang::reentrant]] {
            return sm::detail::atomicScanAndSet(mUsedBits, capacity());
        }

        constexpr AtomicRingQueue(T* storage, std::atomic<uint64_t>* bitset, std::atomic<size_type>* elements, size_type capacity, Allocator allocator) noexcept
            : mAllocator(std::move(allocator))
            , mStorage(storage)
            , mUsedBits(bitset)
            , mElements(elements)
            , mCapacity(capacity + 1)
            , mCount(0)
            , mHead(0)
            , mTail(0) {}

    public:
        constexpr AtomicRingQueue() noexcept = default;

        AtomicRingQueue(const AtomicRingQueue& other) = delete;
        AtomicRingQueue& operator=(const AtomicRingQueue& other) = delete;

        /**
        * @brief Move construct a ring buffer.
        *
        * @note This function is not thread-safe and should only be called when no other threads are accessing the queue.
        *
        * @param other The other ring buffer to move from.
        */
        constexpr AtomicRingQueue(AtomicRingQueue&& other) noexcept
            : mAllocator(std::move(other.mAllocator))
            , mStorage(other.mStorage)
            , mUsedBits(other.mUsedBits)
            , mElements(other.mElements)
            , mCapacity(other.mCapacity)
            , mCount(other.mCount.load())
            , mHead(other.mHead.load())
            , mTail(other.mTail.load()) {
            other.mStorage = nullptr;
            other.mCapacity = 0;
        }

        /**
        * @brief Move assign a ring buffer.
        *
        * @note This function is not thread-safe and should only be called when no other threads are accessing the queue.
        *
        * @param other The other ring buffer to move from.
        * @return The moved ring buffer.
        */
        constexpr AtomicRingQueue& operator=(AtomicRingQueue&& other) noexcept {
            if (this != &other) {
                clear();

                mAllocator = std::move(other.mAllocator);
                mStorage = other.mStorage;
                mUsedBits = other.mUsedBits;
                mElements = other.mElements;
                mCapacity = other.mCapacity;
                mCount.store(other.mCount.load());
                mCount.store(other.mCount.load());
                mHead.store(other.mHead.load());
                mTail.store(other.mTail.load());

                other.mStorage = nullptr;
                other.mCapacity = 0;
            }
            return *this;
        }

        /**
        * @brief Destroy the ring buffer.
        */
        ~AtomicRingQueue() noexcept {
            clear();
        }

        /**
        * @brief Try to push a value onto the queue.
        *
        * Attempts to push a value onto the queue. If the queue is full, the value is not pushed and false is returned.
        * If the value is successfully pushed then @p value is moved from, otherwise it is left unchanged.
        *
        * @param value The value to push.
        *
        * @return true if the value was pushed, false if the queue was full.
        */
        [[nodiscard]]
        bool tryPush(T& value) noexcept [[clang::nonblocking, clang::reentrant]] {
            auto index = allocateElement();
            if (index == (std::numeric_limits<size_t>::max)()) {
                return false;
            }

            //
            // Optimistic increment of count, if we exceed capacity, roll back.
            //
            auto count = mCount.fetch_add(1);
            if (count >= capacity()) {
                mCount.fetch_sub(1);
                sm::detail::atomicClearBit(mUsedBits, index);
                return false;
            }

            std::construct_at(&mStorage[index], std::move(value));

            auto head = mHead.fetch_add(1);
            auto prev = mElements[normalize(head)].exchange(index);
            KM_CHECK(prev == std::numeric_limits<size_type>::max(), "Multiple consumers detected!");

            return true;
        }

        /**
        * @brief Try to pop a value from the queue.
        *
        * Attempts to pop a value from the queue. If the queue is empty, false is returned and @p value is left unchanged.
        * If a value is successfully popped, it is moved into @p value.
        *
        * @param value The value to pop into.
        *
        * @return true if a value was popped, false if the queue was empty.
        */
        [[nodiscard]]
        bool tryPop(T& value) noexcept [[clang::nonblocking, clang::reentrant]] {
            auto index = mElements[normalize(mTail.load())].exchange(std::numeric_limits<size_type>::max());
            if (index == std::numeric_limits<size_type>::max()) {
                return false;
            }

            //
            // Move the value out of storage before we mark the slot as free.
            //
            value = std::move(mStorage[index]);
            std::destroy_at(&mStorage[index]);

            sm::detail::atomicClearBit(mUsedBits, index);

            mTail.fetch_add(1);
            auto count = mCount.fetch_sub(1);
            KM_ASSERT(count > 0);
            return true;
        }

        /**
        * @brief Get an estimate of the number of items in the queue.
        *
        * @warning As this is a lock-free structure the count will be immediately out of date.
        *
        * @param order The memory order to use when loading the count.
        *
        * @return The number of items in the queue.
        */
        size_type count(std::memory_order order = std::memory_order_seq_cst) const noexcept [[clang::nonblocking, clang::reentrant]] {
            return mCount.load(order);
        }

        /**
        * @brief Get the maximum capacity of the queue.
        *
        * @return The maximum number of items the queue can hold.
        */
        size_type capacity() const noexcept [[clang::nonblocking, clang::reentrant]] {
            return mCapacity - 1;
        }

        /**
        * @brief Get the Allocator object used by the ring buffer.
        *
        * @return The allocator.
        */
        allocator_type getAllocator() const noexcept {
            return mAllocator;
        }

        /**
        * @brief Provided for compatibility with standard containers.
        *
        * This is equivalent to `getAllocator()`.
        *
        * @return The allocator.
        */
        allocator_type get_allocator() const noexcept {
            return mAllocator;
        }

        /**
        * @brief Reset the queue to an empty state with the given storage and capacity.
        *
        * @pre @p capacity must be greater than zero.
        * @pre @p storage must point to valid storage of at least @p capacity + 1 elements.
        *
        * The storage must be at least capacity + 1 elements in size. The queue takes ownership of the storage.
        *
        * @note This function is not thread-safe and should only be called when no other threads are accessing the queue.
        *
        * @param storage The storage to use for the queue.
        * @param capacity The maximum number of elements the queue can hold.
        * @param allocator The allocator used to allocate and deallocate the storage.
        */
        void reset(T* storage, size_type capacity, Allocator allocator) noexcept {
            clear();

            mAllocator = std::move(allocator);
            mStorage = storage;
            mCapacity = capacity + 1;
            mCount.store(capacity);
            mCount.store(0);
            mHead.store(0);
            mTail.store(0);
        }

        /**
         * @brief Create a new queue with the given capacity.
         *
         * @param capacity The maximum number of elements the queue can hold.
         * @param queue The created queue.
         * @param allocator The allocator used to allocate and deallocate the storage.
         *
         * @return The status of the operation.
         * @retval OsStatusSuccess The queue was created successfully.
         * @retval OsStatusInvalidInput The capacity was zero.
         * @retval OsStatusOutOfMemory There was not enough memory to create the queue.
         */
        [[nodiscard]]
        static OsStatus create(size_type capacity, AtomicRingQueue<T> *queue [[outparam]], Allocator allocator = Allocator{}) noexcept {
            if (capacity == 0) {
                return OsStatusInvalidInput;
            }

            T* storage = allocator.allocate(capacity + 1);
            if (storage == nullptr) {
                return OsStatusOutOfMemory;
            }

            BitSetAllocator bitsetAllocator{allocator};
            std::atomic<uint64_t> *bitset = bitsetAllocator.allocate(detail::requiredBitsetSize(capacity));
            if (bitset == nullptr) {
                allocator.deallocate(storage, capacity + 1);
                return OsStatusOutOfMemory;
            }

            ElementAllocator elementAllocator{allocator};
            std::atomic<size_type> *elements = elementAllocator.allocate(capacity);
            if (elements == nullptr) {
                allocator.deallocate(storage, capacity + 1);
                bitsetAllocator.deallocate(bitset, detail::requiredBitsetSize(capacity));
                return OsStatusOutOfMemory;
            }

            std::uninitialized_fill_n(bitset, detail::requiredBitsetSize(capacity), 0);
            std::uninitialized_fill_n(elements, capacity, std::numeric_limits<size_type>::max());

            *queue = AtomicRingQueue{storage, bitset, elements, capacity, std::move(allocator)};

            return OsStatusSuccess;
        }
    };
}
