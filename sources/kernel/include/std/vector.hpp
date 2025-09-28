#pragma once

#include <bezos/status.h>

#include <memory>
#include <span>

#include <stddef.h>

#include "allocator/allocator.hpp"
#include "panic.hpp"

namespace stdx {
    template<typename T, typename Allocator = mem::GlobalAllocator<T>>
    class Vector2 {
        [[no_unique_address]] Allocator mAllocator{};

        T *mFront;
        T *mBack;
        T *mCapacity;

        constexpr OsStatus ensureExtra(size_t extra) {
            if ((count() + extra) >= capacity()) {
                return reserveExact(capacity() + extra);
            }

            return OsStatusSuccess;
        }

        constexpr void destroy() {
            if (mFront != nullptr) {
                std::destroy_n(mFront, count());
                mAllocator.deallocate(mFront, capacity() * sizeof(T));
                mFront = nullptr;
                mBack = nullptr;
                mCapacity = nullptr;
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

        constexpr Vector2(Allocator allocator = Allocator{})
            : mAllocator(allocator)
            , mFront(nullptr)
            , mBack(nullptr)
            , mCapacity(nullptr)
        { }

        constexpr Vector2(size_t capacity, Allocator allocator = Allocator{})
            : mAllocator(allocator)
            , mFront(mAllocator.allocate(capacity))
            , mBack(mFront)
            , mCapacity(mFront + capacity)
        { }

        template<typename Iter>
        constexpr Vector2(Iter first, Iter last, Allocator allocator = Allocator{})
            : Vector2(last - first, allocator)
        {
            insert(first, last);
        }

        template<IsRange<const T> R>
        constexpr Vector2(R&& range, Allocator allocator = Allocator{})
            : Vector2(range.begin(), range.end(), allocator)
        { }

        constexpr Vector2(size_t capacity, const T& value, Allocator allocator = Allocator{})
            : Vector2(capacity, allocator)
        {
            std::uninitialized_fill(mFront, mCapacity, value);
        }

        explicit constexpr Vector2(const Vector2& other)
            : Vector2(other.begin(), other.end(), other.getAllocator())
        { }

        constexpr Vector2& operator=(const Vector2& other) {
            if (this != &other) {
                destroy();
                mAllocator = other.getAllocator();
                mFront = mAllocator.allocate(other.count());
                mBack = mFront + other.count();
                mCapacity = mBack;

                std::uninitialized_copy(other.begin(), other.end(), mFront);
            }

            return *this;
        }

        constexpr Vector2(Vector2&& other) noexcept
            : mAllocator(std::exchange(other.mAllocator, Allocator{}))
            , mFront(std::exchange(other.mFront, nullptr))
            , mBack(std::exchange(other.mBack, nullptr))
            , mCapacity(std::exchange(other.mCapacity, nullptr))
        { }

        constexpr Vector2& operator=(Vector2&& other) noexcept {
            if (this != &other) {
                std::swap(mAllocator, other.mAllocator);
                std::swap(mFront, other.mFront);
                std::swap(mBack, other.mBack);
                std::swap(mCapacity, other.mCapacity);
            }

            return *this;
        }

        constexpr ~Vector2() {
            destroy();
        }

        constexpr size_t count() const { return mBack - mFront; }
        constexpr size_t capacity() const { return mCapacity - mFront; }
        constexpr bool isEmpty() const { return mBack == mFront; }

        constexpr Allocator getAllocator() const { return mAllocator; }

        constexpr void clear() {
            std::destroy_n(mFront, count());
            mBack = mFront;
        }

        constexpr OsStatus reserve(size_t size) {
            return reserveExact(std::max(size, capacity()));
        }

        /// @brief Reserve an exact amount of space.
        ///
        /// @param size The size to reserve.
        ///
        /// @retval OsStatusSuccess The reservation was successful.
        /// @retval OsStatusOutOfMemory There was not enough memory to reserve the space.
        ///
        /// @return The status of the operation.
        constexpr OsStatus reserveExact(size_t size) {
            size_t oldCapacity = capacity();
            if (size < oldCapacity) {
                return OsStatusSuccess;
            }

            // need to cache this here for correctness
            size_t currentCount = count();

            if (mFront == nullptr) {
                mFront = mAllocator.allocate(size);
                if (mFront == nullptr) {
                    return OsStatusOutOfMemory;
                }

                mBack = mFront;
                mCapacity = mFront + size;
            } else {
                T *newData = mAllocator.allocate(size);
                if (newData == nullptr) {
                    return OsStatusOutOfMemory;
                }

                std::uninitialized_move(mFront, mBack, newData);
                std::destroy_n(mFront, count());
                mAllocator.deallocate(mFront, oldCapacity);
                mFront = newData;
                mBack = mFront + currentCount;
                mCapacity = mFront + size;
            }

            return OsStatusSuccess;
        }

        constexpr OsStatus resize(size_t size) {
            if (size < count()) {
                std::destroy_n(mFront + size, count() - size);
                mBack = mFront + size;
            } else if (size > count()) {
                if (OsStatus status = ensureExtra(size - count())) {
                    return status;
                }

                std::uninitialized_default_construct(mBack, mFront + size);
                mBack = mFront + size;
            }

            return OsStatusSuccess;
        }

        constexpr T *data() { return mFront; }
        constexpr const T *data() const { return mFront; }

        constexpr T *begin() { return mFront; }
        constexpr T *end() { return mBack; }

        constexpr const T *begin() const { return mFront; }
        constexpr const T *end() const { return mBack; }

        constexpr T& operator[](size_t index) {
            KM_ASSERT(index < count());
            return mFront[index];
        }

        constexpr const T& operator[](size_t index) const {
            KM_ASSERT(index < count());
            return mFront[index];
        }

        constexpr T& front() {
            KM_ASSERT(!isEmpty());
            return *mFront;
        }

        constexpr const T& front() const {
            KM_ASSERT(!isEmpty());
            return *mFront;
        }

        constexpr T& back() {
            KM_ASSERT(!isEmpty());
            return *(mBack - 1);
        }

        constexpr const T& back() const {
            KM_ASSERT(!isEmpty());
            return *(mBack - 1);
        }

        constexpr iterator emplace_back() {
            ensureExtra(1);
            return std::uninitialized_default_construct(mBack++);
        }

        constexpr iterator emplace_back(T value) {
            ensureExtra(1);
            return std::construct_at(mBack++, std::move(value));
        }

        template<typename... Args> requires std::constructible_from<T, Args...>
        constexpr iterator emplace_back(Args&&... args) {
            ensureExtra(1);
            return std::construct_at(mBack++, std::forward<Args>(args)...);
        }

        constexpr OsStatus add(T value) {
            if (OsStatus status = ensureExtra(1)) {
                return status;
            }

            std::construct_at(mBack++, std::move(value));
            return OsStatusSuccess;
        }

        constexpr OsStatus addRange(std::span<const T> src) {
            if (OsStatus status = ensureExtra(src.size())) {
                return status;
            }

            std::uninitialized_copy(src.begin(), src.end(), mBack);
            mBack += src.size();

            return OsStatusSuccess;
        }

        constexpr void pop() {
            KM_ASSERT(!isEmpty());
            std::destroy_at(--mBack);
        }

        template<typename Iter>
        constexpr OsStatus insert(Iter first, Iter last) {
            if (OsStatus status = ensureExtra(last - first)) {
                return status;
            }

            for (auto it = first; it != last; it++) {
                add(*it);
            }

            return OsStatusSuccess;
        }

        constexpr OsStatus insert(size_t index, T value) {
            if (OsStatus status = ensureExtra(1)) {
                return status;
            }

            std::copy_backward(mFront + index, mBack, mBack + 1);
            std::construct_at(mFront + index, std::move(value));
            mBack++;

            return OsStatusSuccess;
        }

        constexpr void remove(size_t index) {
            KM_ASSERT(index < count());

            std::destroy_at(mFront + index);
            std::move(mFront + index + 1, mBack, mFront + index);
            std::destroy_at(--mBack);
        }

        constexpr iterator erase(iterator it) {
            size_t index = it - mFront;
            remove(index);
            return mFront + index;
        }

        constexpr void erase(const T& value) {
            auto it = std::find(mFront, mBack, value);
            if (it != mBack) {
                remove(it - mFront);
            }
        }

        constexpr void erase(size_t first, size_t last) {
            KM_ASSERT(first <= last);
            KM_ASSERT(last <= count());

            std::destroy_n(mFront + first, last - first);
            std::move(mFront + last, mBack, mFront + first);
            mBack -= last - first;
        }

        constexpr friend void swap(Vector2& lhs, Vector2& rhs) {
            std::swap(lhs.mAllocator, rhs.mAllocator);
            std::swap(lhs.mFront, rhs.mFront);
            std::swap(lhs.mBack, rhs.mBack);
            std::swap(lhs.mCapacity, rhs.mCapacity);
        }

        // c++ container compatibility
        constexpr void push_back(T value) { add(std::move(value)); }
        constexpr void pop_back() { pop(); }
    };

    template<typename T>
    using Vector3 = Vector2<T, mem::AllocatorPointer<T>>;
}
