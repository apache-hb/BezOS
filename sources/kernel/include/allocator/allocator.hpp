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

    template<typename A>
    concept Allocator = requires (A allocator, size_t n, size_t align) {
        { allocator.allocate(n) } -> std::same_as<void*>;
        { allocator.allocateAligned(n, align) } -> std::same_as<void*>;
        { allocator.deallocate(std::declval<void*>(), n) } noexcept;
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

#if __cpp_lib_allocate_at_least >= 202302L
        std::allocation_result<void*> allocate_at_least(size_t size) {
            void *ptr = allocate(size);
            return { ptr, ptr ? size : 0 };
        }
#endif
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

#if __cpp_lib_allocate_at_least >= 202302L
        std::allocation_result<T*> allocate_at_least(size_t n) {
            T *ptr = allocate(n);
            return { ptr, ptr ? n : 0 };
        }
#endif
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

#if __cpp_lib_allocate_at_least >= 202302L
        std::allocation_result<T*> allocate_at_least(size_t n) {
            T *memory = allocate(n);
            return { memory, n };
        }
#endif
    };
}

namespace sm {
    /**
     * @brief Replacement for std::allocator that supports nothrow allocations.
     *
     * @tparam T The type to allocate.
     */
    template<typename T>
    class allocator {
    public:
        using value_type = T;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using propagate_on_container_move_assignment = std::true_type;

        constexpr allocator() noexcept = default;

        template <class U>
        constexpr allocator(const allocator<U>&) noexcept { }

        [[nodiscard]]
        constexpr T *allocate(size_t n) noexcept {
            static_assert(sizeof(T) >= 0, "cannot allocate memory for an incomplete type"); // NOLINT(bugprone-sizeof-expression) Allow sizeof on incomplete type to provide better error message
            if (n > std::allocator_traits<allocator>::max_size(*this)) {
                return nullptr;
            }

            return static_cast<T*>(::operator new(n * sizeof(T), std::align_val_t(alignof(T)), std::nothrow));
        }

#if __cpp_lib_allocate_at_least >= 202302L
        [[nodiscard]]
        constexpr std::allocation_result<T*> allocate_at_least(size_t n) noexcept {
            T *memory = allocate(n);
            return { memory, memory ? n : 0 };
        }
#endif

        constexpr void deallocate(T *p, size_t n) noexcept {
            ::operator delete(static_cast<void*>(p), n * sizeof(T), std::align_val_t(alignof(T)));
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

#if __cpp_lib_allocate_at_least >= 202302L
        std::allocation_result<T*> allocate_at_least(size_t n) {
            T *memory = allocate(n);
            return { memory, memory ? n : 0 };
        }
#endif
    };
}

#endif
