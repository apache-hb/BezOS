#pragma once

#include <concepts>
#include <cstdlib>
#include <memory>
#include <utility>

#include <cstring>
#include <cstddef>

extern "C" void *malloc(size_t);
extern "C" void free(void*);

namespace mem {
    class IAllocator {
    public:
        virtual ~IAllocator() = default;

        void operator delete(IAllocator*, std::destroying_delete_t) {
            std::unreachable();
        }

        virtual void *allocate(size_t size) {
            return allocateAligned(size, alignof(std::max_align_t));
        }

        virtual void *allocateAligned(size_t size, size_t align) = 0;
        virtual void deallocate(void *ptr, size_t size) = 0;

        virtual void *reallocate(void *old, size_t oldSize, size_t newSize) {
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
        void deallocate(T *ptr) {
            if (ptr != nullptr) {
                std::destroy_at(ptr);
                deallocate(ptr, sizeof(T));
            }
        }

        template<typename T>
        T *allocateArray(size_t count) {
            if (void *ptr = allocateAligned(sizeof(T) * count, alignof(T))) {
                return new (ptr) T[count]();
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
        void operator()(T *ptr) {
            allocator->deallocate(ptr, 0);
        }

        AllocatorDeleter(IAllocator *allocator) : allocator(allocator) { }
    };

    template<typename T>
    class GlobalAllocator {
    public:
        T *allocate(size_t n) {
            return (T*)malloc(n * sizeof(T));
        }

        void deallocate(T *ptr, size_t) {
            free(ptr);
        }
    };

    template<typename T>
    class AllocatorPointer {
        mem::IAllocator *mAllocator;
    public:
        AllocatorPointer() = delete;
        
        AllocatorPointer(mem::IAllocator *allocator) 
            : mAllocator(allocator) 
        { }

        T *allocate(size_t n) {
            return (T*)mAllocator->allocate(n * sizeof(T));
        }

        void deallocate(T *ptr, size_t n) {
            mAllocator->deallocate(ptr, n * sizeof(T));
        }
    };
}
