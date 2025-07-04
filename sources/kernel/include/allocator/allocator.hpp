#pragma once

#include <cstdlib>
#include <cstring>
#include <cstddef>

#include <memory>
#include <utility>
#include <new>

#include "common/compiler/compiler.hpp"

namespace mem {
    class IAllocator {
    public:
        virtual ~IAllocator() = default;

        void operator delete(IAllocator*, std::destroying_delete_t) {
            std::unreachable();
        }

        virtual void *allocate(size_t size) [[clang::allocating]] {
            return allocateAligned(size, alignof(std::max_align_t));
        }

        virtual void *allocateAligned(size_t size, size_t align) [[clang::allocating]] = 0;
        virtual void deallocate(void *ptr, size_t size) noexcept [[clang::nonallocating]] = 0;

        virtual void *reallocate(void *old, size_t oldSize, size_t newSize) [[clang::allocating]] {
            void *ptr = allocate(newSize);
            if (ptr) {
                std::memcpy(ptr, old, oldSize);
                deallocate(old, oldSize);
            }

            return ptr;
        }

        template<typename T, typename... A>
        T *construct(A&&... args) {
            return allocate<T>(std::forward<A>(args)...);
        }

        template<typename T>
        T *box(T value) {
            return construct<T>(std::move(value));
        }

        template<typename T, typename... A>
        T *allocate(A&&... args) {
            if (void *ptr = allocateAligned(sizeof(T), alignof(T))) {
                return new (ptr) T(std::forward<A>(args)...);
            }

            return nullptr;
        }

        template<typename T>
        void deallocate(T *ptr) noexcept [[clang::nonallocating]] {
            if (ptr != nullptr) {
                std::destroy_at(ptr);
                deallocate(ptr, sizeof(T));
            }
        }

        template<typename T>
        T *allocateArray(size_t count) {
            if (void *ptr = allocateAligned(sizeof(T) * count, alignof(T))) {
                return new (ptr) T[count];
            }

            return nullptr;
        }

        template<typename T> requires (std::is_trivially_move_constructible_v<T>)
        T *reallocateArray(T *old, size_t _, size_t oldCount, size_t newCount) {
            return static_cast<T*>(reallocate(old, sizeof(T) * oldCount, sizeof(T) * newCount));
        }

        template<typename T>
        T *reallocateArray(T *old, size_t usedCount, size_t oldCount, size_t newCount) {
            T *ptr = allocateArray<T>(newCount);
            if (ptr) {
                size_t count = std::min(usedCount, newCount);
                std::uninitialized_move(old, old + count, ptr);
                deallocateArray(old, oldCount);
            }

            return ptr;
        }

        template<typename T>
        void deallocateArray(T *ptr, size_t count) {
            if (ptr != nullptr) {
                std::destroy_n(ptr, count);
                deallocate(ptr, sizeof(T) * count);
            }
        }
    };

    template<typename T>
    class AllocatorDeleter {
        IAllocator *allocator;

    public:
        void operator()(T *ptr) [[clang::allocating]] {
            allocator->deallocate(ptr, 0);
        }

        AllocatorDeleter(IAllocator *allocator) : allocator(allocator) { }
    };

    template<typename T>
    concept Allocator = requires (T it, size_t n, size_t align) {
        { it.allocate(n) } -> std::same_as<void*>;
        { it.allocateAligned(n, align) } -> std::same_as<void*>;
        { it.deallocate(std::declval<T*>(), n) } noexcept;
    };

    class GenericAllocator {
    public:
        void *allocate(size_t size) {
            return std::malloc(size);
        }

        void *allocateAligned(size_t size, size_t align) {
            return std::aligned_alloc(align, size);
        }

        void deallocate(void *ptr, size_t) noexcept {
            std::free(ptr);
        }
    };

    template<typename T>
    class GlobalAllocator {
    public:
        using value_type = T;

        GlobalAllocator() = default;
        GlobalAllocator(GlobalAllocator const&) = default;
        GlobalAllocator& operator=(GlobalAllocator const&) = default;
        GlobalAllocator(GlobalAllocator &&) = default;
        GlobalAllocator& operator=(GlobalAllocator &&) = default;

        template<typename O>
        GlobalAllocator(GlobalAllocator<O> const&) { }

        template<typename O>
        GlobalAllocator(GlobalAllocator<O> &&) { }

        constexpr bool operator==(GlobalAllocator const&) const noexcept {
            return true;
        }

        T *allocate(size_t n) {
            return new (std::nothrow) T[n];
        }

        void deallocate(T *ptr, size_t) noexcept [[clang::nonallocating]] {
            CLANG_DIAGNOSTIC_PUSH();
            CLANG_DIAGNOSTIC_IGNORE("-Wfunction-effects");
            delete[] ptr;
            CLANG_DIAGNOSTIC_POP();
        }
    };

    template<typename T>
    class AllocatorPointer {
        mem::IAllocator *mAllocator;
    public:
        template<typename O>
        friend class AllocatorPointer;

        using value_type = T;
        using size_type = size_t;
        using difference_type = ptrdiff_t;

        template<typename O>
        AllocatorPointer(const AllocatorPointer<O>& other)
            : mAllocator(other.mAllocator)
        { }

        template<typename O>
        AllocatorPointer(AllocatorPointer<O>&& other)
            : mAllocator(other.mAllocator)
        { }

        AllocatorPointer(mem::IAllocator *allocator = nullptr)
            : mAllocator(allocator)
        { }

        T *allocate(size_t n) {
            return (T*)mAllocator->allocateAligned(n * sizeof(T), alignof(T));
        }

        void deallocate(T *ptr, size_t n) noexcept [[clang::nonallocating]] {
            mAllocator->deallocate(ptr, n * sizeof(T));
        }
    };
}

#if !__STDC_HOSTED__ && 0

namespace std {
    template<typename T>
    struct allocator {
        using value_type = T;
        using size_type = size_t;
        using difference_type = ptrdiff_t;

        template<typename O>
        allocator(const allocator<O>&) { }

        template<typename O>
        allocator(allocator<O>&&) { }

        T *allocate(size_t n) {
            return (T*)aligned_alloc(alignof(T), n * sizeof(T));
        }

        void deallocate(T *ptr, size_t) {
            free(ptr);
        }
    };
}

#endif
