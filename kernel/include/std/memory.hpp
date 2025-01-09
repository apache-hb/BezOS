#pragma once

#include "std/vector.hpp"
namespace stdx {
    template<typename Allocator>
    class MemoryResource {
        stdx::Vector<std::byte, Allocator> mMemory;

    public:
        MemoryResource(Allocator allocator)
            : mMemory(allocator)
        { }

        const void *front() const { (void*)mMemory.begin(); }
        const void *back() const { (void*)mMemory.end(); }

        size_t size() const { return mMemory.size(); }

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