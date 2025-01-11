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

        template<typename T>
        size_t add(T value) {
            size_t offset = mMemory.count();
            mMemory.resize(offset + sizeof(T));
            T *ptr = reinterpret_cast<T*>(mMemory.data() + offset);
            std::uninitialized_move_n(&value, 1, ptr);
            return offset;
        }

        void *get(size_t offset) {
            return (void*)(mMemory.data() + offset);
        }

        template<typename T>
        T *get(size_t offset) {
            return reinterpret_cast<T*>(get(offset));
        }
    };
}
