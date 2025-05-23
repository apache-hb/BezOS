#include "memory.hpp"

#include <format>
#include <memory_resource>
#include <vector>

#include "common/compiler/compiler.hpp"

CLANG_DIAGNOSTIC_PUSH();
CLANG_DIAGNOSTIC_IGNORE("-Wfunction-effects"); // We don't care about realtime effects in tests

static km::PageMemoryTypeLayout GetDefaultPatLayout(void) {
    enum {
        kEntryWriteBack,
        kEntryWriteThrough,
        kEntryUncachedOverridable,
        kEntryUncached,
        kEntryWriteCombined,
        kEntryWriteProtect,
    };

    return km::PageMemoryTypeLayout {
        .deferred = kEntryUncachedOverridable,
        .uncached = kEntryUncached,
        .writeCombined = kEntryWriteCombined,
        .writeThrough = kEntryWriteThrough,
        .writeProtect = kEntryWriteProtect,
        .writeBack = kEntryWriteBack,
    };
}

struct BufferAllocator final : public mem::IAllocator {
    std::pmr::monotonic_buffer_resource mResource;

public:
    BufferAllocator(uint8_t *memory, size_t size)
        : mResource(memory, size)
    { }

    void *allocateAligned(size_t size, size_t align) override {
        return mResource.allocate(size, align);
    }

    void deallocate(void *ptr, size_t size) noexcept override {
        mResource.deallocate(ptr, size);
    }
};

CLANG_DIAGNOSTIC_POP();

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
    km::AddressMapping pteMemory;
    km::PageBuilder pm;

    SystemMemoryTestBody(size_t size = 0x10000)
        : allocatorMemory { (uint8_t*)_mm_malloc(size, 0x1000), &_mm_free }
        , pteMemory { allocatorMemory.get(), (uintptr_t)allocatorMemory.get(), size }
        , pm { 48, 48, GetDefaultPatLayout() }
    { }

    km::MemoryRange addSegment(size_t size, boot::MemoryRegion::Type type) {
        void *ptr = _mm_malloc(size, 0x1000);
        mmUniquePtr<std::byte[]> segment { (std::byte*)ptr, &_mm_free };
        memory.push_back({std::move(segment), size, type});
        return km::MemoryRange {(uintptr_t)ptr, (uintptr_t)ptr + size};
    }

    km::SystemMemory make(size_t systemArea = 0x10000) {
        std::vector<boot::MemoryRegion> regions;
        for (const MemorySegment& seg : memory) {
            regions.push_back(seg.region());
        }

        std::ranges::sort(regions, [](const boot::MemoryRegion& a, const boot::MemoryRegion& b) {
            return a.range.front < b.range.front;
        });

        auto systemSegment = addSegment(systemArea, boot::MemoryRegion::eUsable);

        uintptr_t dataFront = systemSegment.front.address;
        uintptr_t dataBack = systemSegment.back.address;
        km::VirtualRangeEx system{dataFront, dataBack};
        km::SystemMemory result;
        if (OsStatus status = km::SystemMemory::create(regions, system, pm, pteMemory, &result)) {
            throw std::runtime_error(std::format("Failed to create SystemMemory: {}", status));
        }
        return result;
    }

    km::SystemMemory makeNoAlloc() {
        std::vector<boot::MemoryRegion> regions;
        for (const MemorySegment& seg : memory) {
            regions.push_back(seg.region());
        }

        std::ranges::sort(regions, [](const boot::MemoryRegion& a, const boot::MemoryRegion& b) {
            return a.range.front < b.range.front;
        });

        uintptr_t dataFront = -(1ull << (48 - 1));
        uintptr_t dataBack = -(1ull << (48 - 2));
        km::VirtualRangeEx system{dataFront, dataBack};
        km::SystemMemory result;
        if (OsStatus status = km::SystemMemory::create(regions, system, pm, pteMemory, &result)) {
            throw std::runtime_error(std::format("Failed to create SystemMemory: {}", status));
        }
        return result;
    }
};
