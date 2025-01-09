#pragma once

#include <concepts>
#include <memory>
#include <stddef.h>
#include <utility>

namespace mem {
    template<typename Super>
    class AllocatorMixin {
        Super& super() {
            return static_cast<Super&>(*this);
        }

    public:
        template<typename T, typename... A> requires std::is_constructible_v<T, A...>
        T *allocate(A&&... args) {
            if (void *ptr = super().allocate(sizeof(T), alignof(T))) {
                return new (ptr) T(std::forward<A>(args)...);
            }

            return nullptr;
        }

        template<typename T>
        void deallocate(T *ptr) {
            if (ptr != nullptr) {
                std::destroy_at(ptr);
                super().deallocate(ptr, sizeof(T));
            }
        }

        template<typename T>
        T *allocateArray(size_t count) {
            if (void *ptr = super().allocate(sizeof(T) * count, alignof(T))) {
                return new (ptr) T[count]();
            }

            return nullptr;
        }

        template<typename T>
        void deallocateArray(T *ptr, size_t count) {
            if (ptr != nullptr) {
                std::destroy_n(ptr, count);
                super().deallocate(ptr, sizeof(T) * count);
            }
        }
    };

    template<typename Allocator>
    class AllocatorPointer : public AllocatorMixin<AllocatorPointer<Allocator>> {
        Allocator *mAllocator;

    public:
        AllocatorPointer(Allocator *allocator)
            : mAllocator(allocator)
        { }

        void *allocate(size_t size, size_t align) {
            return mAllocator->allocate(size, align);
        }

        void deallocate(void *ptr, size_t size) {
            mAllocator->deallocate(ptr, size);
        }
    };
}
