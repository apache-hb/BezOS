#pragma once

#include <algorithm>
#include <bezos/status.h>

#include "common/util/util.hpp"
#include "allocator/allocator.hpp"

namespace sm {
    template<typename T, size_t N, typename Allocator = mem::GlobalAllocator<T>>
    class InlinedVector {
        static_assert(N > 0, "InlinedVector size must be greater than 0");

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
        T *mBack;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfunction-effects" // Deallocating is alright in this context

        void deallocate(T *ptr, size_t size) noexcept [[clang::nonallocating]] {
            mAllocator.deallocate(ptr, size * sizeof(T));
        }

#pragma clang diagnostic pop

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

        InlinedVector() noexcept
            : InlinedVector(Allocator{})
        { }

        ~InlinedVector() noexcept [[clang::nonallocating]] {
            destroy();
        }

        InlinedVector(Allocator allocator) noexcept
            : mAllocator(allocator)
            , mBack(mStorage.flat.begin())
        { }

        InlinedVector(InlinedVector&& other) noexcept
            : mAllocator(std::move(other.mAllocator))
            , mStorage(other.mStorage)
            , mBack(other.mBack)
        {
            other.clear();
        }

        InlinedVector& operator=(InlinedVector&& other) noexcept {
            if (this != &other) {
                clear();
                mAllocator = std::move(other.mAllocator);
                mStorage = other.mStorage;
                mBack = other.mBack;
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
            const char *back = static_cast<char*>((void*)mBack);
            const char *data = static_cast<char*>((void*)&mStorage);
            return !(back >= data && back < (data + sizeof(Storage)));
        }

        OsStatus ensureExtra(size_t extra) noexcept [[clang::allocating]] {
            if ((count() + extra) >= capacity()) {
                return reserveExact(capacity() + extra);
            }

            return OsStatusSuccess;
        }

        T *end() noexcept [[clang::nonblocking]] { return mBack; }
        const T *end() const noexcept [[clang::nonblocking]] { return mBack; }

        size_t count() const noexcept [[clang::nonblocking]] { return end() - begin(); }
        size_t capacity() const noexcept [[clang::nonblocking]] { return getCapacity() - begin(); }
        size_t isEmpty() const noexcept [[clang::nonblocking]] { return end() == begin(); }
        size_t inlineCapacity() const noexcept [[clang::nonblocking]] { return N; }

        Allocator getAllocator() const noexcept [[clang::nonblocking]] { return mAllocator; }

        void clear() noexcept [[clang::nonallocating]] {
            std::destroy(begin(), end());
            mBack = begin();
        }

        T *data() noexcept [[clang::nonblocking]] { return begin(); }
        const T *data() const noexcept [[clang::nonblocking]] { return begin(); }

        T& front() noexcept [[clang::nonblocking]] { return *begin(); }
        const T& front() const noexcept [[clang::nonblocking]] { return *begin(); }

        T& back() noexcept [[clang::nonblocking]] { return *(end() - 1); }
        const T& back() const noexcept [[clang::nonblocking]] { return *(end() - 1); }

        T& operator[](size_t index) noexcept [[clang::nonblocking]] { return *(begin() + index); }
        const T& operator[](size_t index) const noexcept [[clang::nonblocking]] { return *(begin() + index); }

        OsStatus reserveExact(size_t newCapacity) noexcept [[clang::allocating]] {
            size_t oldCapacity = capacity();
            if (newCapacity <= oldCapacity) {
                return OsStatusSuccess;
            }

            size_t currentCount = count();

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
                mBack = newData + currentCount;
            } else if (newCapacity > inlineCapacity()) {
                T *newData = mAllocator.allocate(newCapacity * sizeof(T));
                if (newData == nullptr) {
                    return OsStatusOutOfMemory;
                }

                std::uninitialized_move(begin(), end(), newData);
                std::destroy(begin(), end());

                mStorage.heap.data = newData;
                mStorage.heap.capacity = newData + newCapacity;
                mBack = newData + currentCount;
            }

            return OsStatusSuccess;
        }

        OsStatus reserve(size_t size) noexcept [[clang::allocating]] {
            return reserveExact(std::max(size, capacity()));
        }

        OsStatus resize(size_t size) noexcept [[clang::allocating]] {
            if (size < count()) {
                std::destroy_n(mBack, count() - size);
                mBack = begin() + size;
            } else if (size > count()) {
                if (OsStatus status = ensureExtra(size - count())) {
                    return status;
                }

                std::uninitialized_default_construct(mBack, begin() + size);
                mBack = begin() + size;
            }

            return OsStatusSuccess;
        }

        OsStatus add(T value) noexcept [[clang::allocating]] {
            if (OsStatus status = ensureExtra(1)) {
                return status;
            }

            std::construct_at(mBack++, std::move(value));
            return OsStatusSuccess;
        }

        void remove(size_t index) noexcept [[clang::nonallocating]] {
            std::destroy_at(begin() + index);
            std::move(begin() + index + 1, end(), begin() + index);
            std::destroy_at(--mBack);
        }

        friend void swap(InlinedVector& lhs, InlinedVector& rhs) noexcept [[clang::nonallocating]] {
            std::swap(lhs.mAllocator, rhs.mAllocator);
            std::swap(lhs.mStorage, rhs.mStorage);
            std::swap(lhs.mBack, rhs.mBack);
        }
    };
}
