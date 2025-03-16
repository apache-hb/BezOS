#pragma once

#include "allocator/allocator.hpp"
#include "std/string_view.hpp"

namespace stdx {
    template<typename T, typename Allocator = mem::GlobalAllocator<T>>
    class StringBase {
        [[no_unique_address]] Allocator mAllocator;

        T *mFront;
        T *mBack;
        T *mCapacity;

        constexpr void destroy() {
            if (mFront) {
                mAllocator.deallocate(mFront, capacity());
            }

            mFront = nullptr;
            mBack = nullptr;
            mCapacity = nullptr;
        }

        constexpr void ensureExtra(size_t extra) {
            if ((count() + extra) >= capacity()) {
                reserveExact(capacity() + extra + 1); // +1 for null terminator
            }
        }

    public:
        using iterator = T*;
        using const_iterator = const T*;
        using reference = T&;
        using const_reference = const T&;
        using value_type = T;
        using allocator_type = Allocator;
        using pointer = T*;
        using const_pointer = const T*;

        [[nodiscard]]
        constexpr StringBase(Allocator allocator = Allocator{})
            : mAllocator(allocator)
            , mFront(nullptr)
            , mBack(nullptr)
            , mCapacity(nullptr)
        { }

        template<typename Iter>
        [[nodiscard]]
        constexpr StringBase(Iter first, Iter last, Allocator allocator = Allocator{})
            : StringBase(allocator)
        {
            insert(first, last);
        }

        template<size_t N>
        [[nodiscard]]
        constexpr StringBase(const T (&text)[N], Allocator allocator = Allocator{})
            : StringBase(text, text + N - 1, allocator)
        { }

        [[nodiscard]]
        explicit constexpr StringBase(stdx::StringViewBase<T> view, Allocator allocatr = Allocator{})
            : StringBase(view.begin(), view.end(), allocatr)
        { }

        [[nodiscard]]
        explicit constexpr StringBase(const StringBase& other)
            : StringBase(other.begin(), other.end(), other.getAllocator())
        { }

        constexpr StringBase& operator=(const StringBase& other) {
            if (this != &other) {
                clear();
                insert(other.begin(), other.end());
            }

            return *this;
        }

        [[nodiscard]]
        constexpr StringBase(StringBase&& other)
            : mAllocator(std::exchange(other.mAllocator, Allocator{}))
            , mFront(std::exchange(other.mFront, nullptr))
            , mBack(std::exchange(other.mBack, nullptr))
            , mCapacity(std::exchange(other.mCapacity, nullptr))
        { }

        constexpr StringBase& operator=(StringBase&& other) {
            if (this != &other) {
                destroy();

                mAllocator = std::exchange(other.mAllocator, Allocator{});
                mFront = std::exchange(other.mFront, nullptr);
                mBack = std::exchange(other.mBack, nullptr);
                mCapacity = std::exchange(other.mCapacity, nullptr);
            }

            return *this;
        }

        constexpr ~StringBase() {
            destroy();
        }

        constexpr void reserve(size_t size) {
            reserveExact(std::max(size, capacity()));
        }

        constexpr void reserveExact(size_t size) {
            size_t oldCapacity = capacity();
            if (size < oldCapacity) {
                return;
            }

            size_t currentCount = count();

            if (mFront == nullptr) {
                mFront = mAllocator.allocate(size);
                mBack = mFront;
                mCapacity = mFront + size;
            } else {
                T *newData = mAllocator.allocate(size);
                std::uninitialized_move(mFront, mBack, newData);
                std::destroy_n(mFront, count());
                mAllocator.deallocate(mFront, oldCapacity);
                mFront = newData;
                mBack = mFront + currentCount;
                mCapacity = mFront + size;
            }
        }

        constexpr void resize(size_t size) {
            if (size < count()) {
                std::destroy_n(mFront + size, count() - size);
                mBack = mFront + size;
            } else if (size > count()) {
                ensureExtra(size - count());
                std::uninitialized_default_construct(mBack, mFront + size);
                mBack = mFront + size;
            }
        }

        constexpr size_t count() const { return mBack - mFront; }
        constexpr size_t capacity() const { return mCapacity - mFront; }
        constexpr bool isEmpty() const { return mBack == mFront; }

        constexpr Allocator getAllocator() const { return mAllocator; }

        constexpr void add(T value) {
            ensureExtra(1);
            std::construct_at(mBack++, std::move(value));
            mBack[0] = T{};
        }

        constexpr void pop() {
            std::destroy_at(--mBack);
            mBack[0] = T{};
        }

        template<typename Iter>
        constexpr void insert(Iter first, Iter last) {
            ensureExtra(last - first);

            std::uninitialized_move(first, last, mBack);
            mBack += last - first;
            mBack[0] = T{};
        }

        constexpr void append(const T *text, size_t size) {
            insert(text, text + size);
        }

        constexpr void append(const T *text) {
            append(text, std::char_traits<T>::length(text));
        }

        template<typename R> requires (IsRange<const T, R>)
        constexpr void append(R&& range) {
            insert(std::begin(range), std::end(range));
        }

        constexpr void remove(size_t index) {
            std::destroy_at(mFront + index);
            std::copy(mFront + index + 1, mBack, mFront + index);
            std::destroy_at(--mBack);
            mBack[0] = T{};
        }

        constexpr void clear() {
            if (!isEmpty()) {
                std::destroy(mFront, mBack);
                mBack = mFront;
                mBack[0] = T{};
            }
        }

        constexpr T *begin() { return mFront; }
        constexpr const T *begin() const { return mFront; }

        constexpr T *end() { return mBack; }
        constexpr const T *end() const { return mBack; }

        constexpr T& front() { return *mFront; }
        constexpr const T& front() const { return *mFront; }

        constexpr T& back() { return *(mBack - 1); }
        constexpr const T& back() const { return *(mBack - 1); }

        constexpr T *data() { return mFront; }
        constexpr const T *data() const { return mFront; }

        constexpr friend StringBase operator+(const StringBase& lhs, const StringBase& rhs) {
            StringBase result(lhs);
            result.insert(rhs.begin(), rhs.end());
            return result;
        }

        constexpr T *cString() {
            ensureExtra(1);
            mBack[0] = T{};
            return mFront;
        }

        constexpr const T *cString() const {
            ensureExtra(1);
            mBack[0] = T{};
            return mFront;
        }

        constexpr T& operator[](size_t index) {
            return mFront[index];
        }

        constexpr const T& operator[](size_t index) const {
            return mFront[index];
        }

        friend constexpr auto operator<=>(const StringBase& lhs, const StringBase& rhs) {
            return std::lexicographical_compare_three_way(std::begin(lhs), std::end(lhs), std::begin(rhs), std::end(rhs));
        }

        friend constexpr bool operator==(const StringBase& lhs, const StringBase& rhs) {
            return std::equal(std::begin(lhs), std::end(lhs), std::begin(rhs), std::end(rhs));
        }

        friend constexpr void swap(StringBase& lhs, StringBase& rhs) {
            std::swap(lhs.mAllocator, rhs.mAllocator);
            std::swap(lhs.mFront, rhs.mFront);
            std::swap(lhs.mBack, rhs.mBack);
            std::swap(lhs.mCapacity, rhs.mCapacity);
        }
    };

    using String = StringBase<char>;
}
