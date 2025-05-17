#pragma once

#include <algorithm>
#include <bezos/status.h>

#include "common/util/util.hpp"
#include "allocator/allocator.hpp"

namespace sm {
    template<typename T, size_t N, typename Allocator = mem::GlobalAllocator<T>>
    class InlinedVector {
        static_assert(N > 0, "InlinedVector size must be greater than 0");

        static constexpr size_t kHeapFlag = (1zu << (sizeof(size_t) * 8 - 1));

        [[no_unique_address]] Allocator mAllocator{};

        struct InlineStorage {
            alignas(T) char data[sizeof(T) * N];

            T *begin() noexcept { return reinterpret_cast<T*>(data); }
            const T *begin() const noexcept { return reinterpret_cast<const T*>(data); }

            T *end() noexcept { return begin() + N; }
            const T *end() const noexcept { return begin() + N; }
        };

        struct HeapStorage {
            T *data;
            T *capacity;
        };

        union Storage {
            InlineStorage flat;
            HeapStorage heap;
        };

        Storage mStorage;
        size_t mSize;

        void deallocate(T *ptr, size_t size) noexcept [[clang::nonallocating]] {
            mAllocator.deallocate(ptr, size * sizeof(T));
        }

        T *getCapacity() noexcept [[clang::nonblocking]] {
            if (isHeapAllocated()) {
                return mStorage.heap.capacity;
            } else {
                return mStorage.flat.end();
            }
        }

        const T *getCapacity() const noexcept [[clang::nonblocking]] {
            if (isHeapAllocated()) {
                return mStorage.heap.capacity;
            } else {
                return mStorage.flat.end();
            }
        }

        void destroy() noexcept [[clang::nonallocating]] {
            std::destroy(begin(), end());
            if (isHeapAllocated()) {
                deallocate(mStorage.heap.data, capacity());
            }
        }

        void setSize(size_t newSize) noexcept [[clang::nonblocking]] {
            mSize = (mSize & kHeapFlag) | newSize;
        }

    public:
        using value_type = T;
        using allocator_type = Allocator;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;
        using iterator = T*;
        using const_iterator = const T*;

        UTIL_NOCOPY(InlinedVector);

        constexpr InlinedVector() noexcept [[clang::nonallocating]]
            : InlinedVector(Allocator{})
        { }

        constexpr ~InlinedVector() noexcept [[clang::nonallocating]] {
            destroy();
        }

        constexpr InlinedVector(Allocator allocator) noexcept
            : mAllocator(allocator)
            , mSize(0)
        { }

        InlinedVector(InlinedVector&& other) noexcept [[clang::nonallocating]]
            : mAllocator(std::move(other.mAllocator))
            , mStorage(other.mStorage)
            , mSize(other.mSize)
        {
            other.mSize = 0;
        }

        template<size_t M> requires (M <= N)
        InlinedVector(const T (&array)[M]) noexcept [[clang::nonallocating]]
            : mAllocator(Allocator{})
            , mSize(0)
        {
            std::move(std::begin(array), std::end(array), begin());
            mSize += M;
        }

        InlinedVector& operator=(InlinedVector&& other) noexcept [[clang::nonallocating]] {
            if (this != &other) {
                clear();
                mAllocator = std::move(other.mAllocator);
                mStorage = other.mStorage;
                mSize = other.mSize;
                other.clear();
            }
            return *this;
        }

        T *begin() noexcept [[clang::nonblocking]] {
            if (isHeapAllocated()) {
                return mStorage.heap.data;
            } else {
                return mStorage.flat.begin();
            }
        }

        const T *begin() const noexcept [[clang::nonblocking]] {
            if (isHeapAllocated()) {
                return mStorage.heap.data;
            } else {
                return mStorage.flat.begin();
            }
        }

        bool isHeapAllocated() const noexcept [[clang::nonblocking]] {
            return mSize & kHeapFlag;
        }

        OsStatus ensureExtra(size_t extra) noexcept [[clang::allocating]] {
            if ((count() + extra) >= capacity()) {
                return reserveExact(capacity() + extra);
            }

            return OsStatusSuccess;
        }

        T *end() noexcept [[clang::nonblocking]] { return begin() + count(); }
        const T *end() const noexcept [[clang::nonblocking]] { return begin() + count(); }

        size_t count() const noexcept [[clang::nonblocking]] { return mSize & ~kHeapFlag; }
        size_t capacity() const noexcept [[clang::nonblocking]] { return getCapacity() - begin(); }
        size_t isEmpty() const noexcept [[clang::nonblocking]] { return end() == begin(); }
        size_t inlineCapacity() const noexcept [[clang::nonblocking]] { return N; }

        Allocator getAllocator() const noexcept [[clang::nonblocking]] { return mAllocator; }

        void clear() noexcept [[clang::nonallocating]] {
            std::destroy(begin(), end());
            setSize(0);
        }

        T *data() noexcept [[clang::nonblocking]] { return begin(); }
        const T *data() const noexcept [[clang::nonblocking]] { return begin(); }

        T& front() noexcept [[clang::nonblocking]] { return *begin(); }
        const T& front() const noexcept [[clang::nonblocking]] { return *begin(); }

        T& back() noexcept [[clang::nonblocking]] { return *(end() - 1); }
        const T& back() const noexcept [[clang::nonblocking]] { return *(end() - 1); }

        T& at(size_t index) noexcept [[clang::nonblocking]] { return *(begin() + index); }
        const T& at(size_t index) const noexcept [[clang::nonblocking]] { return *(begin() + index); }

        T& operator[](size_t index) noexcept [[clang::nonblocking]] { return *(begin() + index); }
        const T& operator[](size_t index) const noexcept [[clang::nonblocking]] { return *(begin() + index); }

        OsStatus reserveExact(size_t newCapacity) noexcept [[clang::allocating]] {
            size_t oldCapacity = capacity();
            if (newCapacity <= oldCapacity) {
                return OsStatusSuccess;
            }

            if (isHeapAllocated()) {
                T *newData = mAllocator.allocate(newCapacity * sizeof(T));
                if (newData == nullptr) {
                    return OsStatusOutOfMemory;
                }

                std::uninitialized_move(begin(), end(), newData);
                std::destroy(begin(), end());
                deallocate(mStorage.heap.data, oldCapacity);

                mStorage.heap.data = newData;
                mStorage.heap.capacity = newData + newCapacity;
            } else if (newCapacity > inlineCapacity()) {
                T *newData = mAllocator.allocate(newCapacity * sizeof(T));
                if (newData == nullptr) {
                    return OsStatusOutOfMemory;
                }

                std::uninitialized_move(begin(), end(), newData);
                std::destroy(begin(), end());

                mStorage.heap.data = newData;
                mStorage.heap.capacity = newData + newCapacity;
                mSize |= kHeapFlag;
            }

            return OsStatusSuccess;
        }

        OsStatus reserve(size_t size) noexcept [[clang::allocating]] {
            return reserveExact(std::max(size, capacity()));
        }

        OsStatus resize(size_t size) noexcept [[clang::allocating]] {
            if (size < count()) {
                std::destroy_n(end(), count() - size);
                setSize(size);
            } else if (size > count()) {
                if (OsStatus status = ensureExtra(size - count())) {
                    return status;
                }

                std::uninitialized_default_construct(end(), begin() + size);
                setSize(size);
            }

            return OsStatusSuccess;
        }

        OsStatus add(T value) noexcept [[clang::allocating]] {
            if (OsStatus status = ensureExtra(1)) {
                return status;
            }

            std::construct_at(end(), std::move(value));
            setSize(count() + 1);
            return OsStatusSuccess;
        }

        OsStatus add(const T *first, const T *last) noexcept [[clang::allocating]] {
            size_t extra = last - first;
            if (OsStatus status = ensureExtra(extra)) {
                return status;
            }

            std::uninitialized_copy(first, last, end());
            setSize(count() + extra);
            return OsStatusSuccess;
        }

        void remove(size_t index) noexcept [[clang::nonallocating]] {
            std::destroy_at(begin() + index);
            std::move(begin() + index + 1, end(), begin() + index);
            std::destroy_at(end());
            setSize(count() - 1);
        }

        void remove_if(T element) noexcept [[clang::nonallocating]] {
            for (size_t i = 0; i < count(); i++) {
                if (at(i) == element) {
                    remove(i);
                    i--;
                }
            }
        }

        friend void swap(InlinedVector& lhs, InlinedVector& rhs) noexcept [[clang::nonallocating]] {
            std::swap(lhs.mAllocator, rhs.mAllocator);
            std::swap(lhs.mStorage, rhs.mStorage);
            std::swap(lhs.mSize, rhs.mSize);
        }
    };
}
