#pragma once

#include "std/vector.hpp"
namespace stdx {
    class MemoryResource {
        stdx::Vector<std::byte> mMemory;

    public:
        MemoryResource(mem::IAllocator *allocator)
            : mMemory(allocator)
        { }

        const void *front() const { return (void*)mMemory.begin(); }
        const void *back() const { return (void*)mMemory.end(); }

        size_t count() const { return mMemory.count(); }

        size_t add(const void *data, size_t size) {
            std::span<const std::byte> span((const std::byte*)data, size);
            size_t offset = mMemory.count();
            mMemory.addRange(span);
            return offset;
        }

        template<typename T>
        size_t add(const T& value) {
            return add(reinterpret_cast<const void*>(&value), sizeof(T));
        }

        void *get(size_t offset) {
            return (void*)(mMemory.begin() + offset);
        }

        template<typename T>
        T *get(size_t offset) {
            return reinterpret_cast<T*>(get(offset));
        }
    };
}