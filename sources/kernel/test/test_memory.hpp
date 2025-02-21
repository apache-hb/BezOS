#include <memory_resource>

#include "memory.hpp"

struct BufferAllocator final : public mem::IAllocator {
    std::pmr::monotonic_buffer_resource mResource;

public:
    BufferAllocator(uint8_t *memory, size_t size)
        : mResource(memory, size)
    { }

    void *allocateAligned(size_t size, size_t align) override {
        return mResource.allocate(size, align);
    }

    void deallocate(void *ptr, size_t size) override {
        mResource.deallocate(ptr, size);
    }
};

template<typename T>
using mmUniquePtr = std::unique_ptr<T, decltype(&_mm_free)>;

struct MemorySegment {
    mmUniquePtr<std::byte[]> memory;
    size_t size;
    boot::MemoryRegion::Type type;

    boot::MemoryRegion region() const {
        return boot::MemoryRegion { type, km::MemoryRange { (uintptr_t)memory.get(), (uintptr_t)memory.get() + size } };
    }
};

struct SystemMemoryTestBody {
    mmUniquePtr<uint8_t[]> allocatorMemory;
    std::vector<MemorySegment> memory;
    boot::MemoryMap memmap;
    BufferAllocator allocator;
    km::PageBuilder pm;

    SystemMemoryTestBody()
        : allocatorMemory { (uint8_t*)_mm_malloc(0x10000, 0x1000), &_mm_free }
        , allocator { allocatorMemory.get(), 0x10000 }
        , pm { 48, 48, 0, km::PageMemoryTypeLayout { 2, 3, 4, 1, 5, 0 } }
    { }

    km::MemoryRange addSegment(size_t size, boot::MemoryRegion::Type type) {
        void *ptr = _mm_malloc(size, 0x1000);
        mmUniquePtr<std::byte[]> segment { (std::byte*)ptr, &_mm_free };
        memory.push_back({std::move(segment), size, type});
        return km::MemoryRange {(uintptr_t)ptr, (uintptr_t)ptr + size};
    }

    km::SystemMemory make() {
        std::vector<boot::MemoryRegion> regions;
        for (const MemorySegment& seg : memory) {
            regions.push_back(seg.region());
        }

        uintptr_t dataFront = -(1ull << (48 - 1));
        uintptr_t dataBack = -(1ull << (48 - 2));
        return km::SystemMemory(boot::MemoryMap{regions}, km::VirtualRange{(void*)dataFront, (void*)dataBack}, pm, &allocator);
    }
};
