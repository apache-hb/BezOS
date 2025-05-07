#pragma once

#include "memory/heap.hpp"
#include "memory/paging.hpp"
#include "memory/pte.hpp"
#include "memory/range.hpp"

#include <type_traits>

#include "common/compiler/compiler.hpp"

namespace km {
    class MappingAllocation;
    class StackMappingAllocation;

    namespace detail {
        template<typename T>
        static constexpr PageFlags kDefaultPageFlags = std::is_const_v<T> ? PageFlags::eRead : PageFlags::eData;
    }

    struct AddressSpaceStats {
        TlsfHeapStats heap;
    };

    class AddressSpace {
        stdx::SpinLock mLock;

        PageTables mTables;
        TlsfHeap mVmemHeap GUARDED_BY(mLock);

    public:
        UTIL_NOCOPY(AddressSpace);

        constexpr AddressSpace(AddressSpace&& other) noexcept
            : mTables(std::move(other.mTables))
            , mVmemHeap(std::move(other.mVmemHeap))
        { }

        constexpr AddressSpace& operator=(AddressSpace&& other) noexcept {
            CLANG_DIAGNOSTIC_PUSH();
            CLANG_DIAGNOSTIC_IGNORE("-Wthread-safety");

            mTables = std::move(other.mTables);
            mVmemHeap = std::move(other.mVmemHeap);
            return *this;

            CLANG_DIAGNOSTIC_POP();
        }

        constexpr AddressSpace() noexcept = default;

        void reserve(VirtualRange range);

        [[nodiscard]]
        OsStatus unmap(VirtualRange range);

        [[nodiscard]]
        void *map(MemoryRange range, PageFlags flags = PageFlags::eData, MemoryType type = MemoryType::eWriteBack);

        [[nodiscard]]
        OsStatus map(MemoryRange range, const void *hint, PageFlags flags, MemoryType type, AddressMapping *mapping);

        [[nodiscard]]
        OsStatus map(MemoryRange range, PageFlags flags, MemoryType type, AddressMapping *mapping) {
            return map(range, nullptr, flags, type, mapping);
        }

        [[nodiscard]]
        OsStatus mapStack(MemoryRange range, PageFlags flags, StackMapping *mapping);

        [[nodiscard]]
        OsStatus unmapStack(StackMapping mapping);

        template<typename T> requires (std::is_standard_layout_v<T>)
        [[nodiscard]]
        T *mapObject(MemoryRange range, PageFlags flags = detail::kDefaultPageFlags<T>, MemoryType type = MemoryType::eWriteBack) {
            return (T*)map(range, flags, type);
        }

        template<typename T> requires (std::is_standard_layout_v<T>)
        [[nodiscard]]
        T *mapObject(PhysicalAddress paddr, PageFlags flags = detail::kDefaultPageFlags<T>, MemoryType type = MemoryType::eWriteBack) {
            return mapObject<T>(MemoryRange::of(paddr, sizeof(T)), flags, type);
        }

        template<typename T> requires (std::is_standard_layout_v<T>)
        [[nodiscard]]
        const T *mapConst(MemoryRange range, MemoryType type = MemoryType::eWriteBack) {
            return (const T*)map(range, PageFlags::eRead, type);
        }

        template<typename T> requires (std::is_standard_layout_v<T>)
        [[nodiscard]]
        const T *mapConst(PhysicalAddress paddr, MemoryType type = MemoryType::eWriteBack) {
            return mapConst<T>(MemoryRange::of(paddr, sizeof(T)), type);
        }

        template<typename T> requires (std::is_standard_layout_v<T>)
        [[nodiscard]]
        T *mapMmio(PhysicalAddress paddr, PageFlags flags = detail::kDefaultPageFlags<T>) {
            return mapObject<T>(MemoryRange::of(paddr, sizeof(T)), flags, MemoryType::eUncached);
        }

        template<typename T> requires (std::is_standard_layout_v<T>)
        [[nodiscard]]
        T *mapMmio(MemoryRange range, PageFlags flags = detail::kDefaultPageFlags<T>) {
            return mapObject<T>(range, flags, MemoryType::eUncached);
        }

#pragma region New address space api

        void *mapGenericObject(MemoryRangeEx range, PageFlags flags, MemoryType type, TlsfAllocation *allocation [[gnu::nonnull]]);

        template<typename T> requires (std::is_standard_layout_v<T>)
        [[nodiscard]]
        T *mapObject(MemoryRangeEx range, PageFlags flags, MemoryType type, TlsfAllocation *allocation [[gnu::nonnull]]) {
            return (T*)mapGenericObject(range, flags, type, allocation);
        }

        template<typename T> requires (std::is_standard_layout_v<T>)
        [[nodiscard]]
        T *mapObject(PhysicalAddressEx paddr, PageFlags flags, MemoryType type, TlsfAllocation *allocation [[gnu::nonnull]]) {
            return mapObject<T>(MemoryRangeEx::of(paddr, sizeof(T)), flags, type, allocation);
        }

        template<typename T> requires (std::is_standard_layout_v<T>)
        [[nodiscard]]
        const T *mapConst(MemoryRangeEx range, TlsfAllocation *allocation [[gnu::nonnull]]) {
            return (const T*)mapGenericObject(range, PageFlags::eRead, MemoryType::eWriteBack, allocation);
        }

        template<typename T> requires (std::is_standard_layout_v<T>)
        [[nodiscard]]
        const T *mapConst(PhysicalAddressEx paddr, TlsfAllocation *allocation [[gnu::nonnull]]) {
            return mapConst<T>(MemoryRangeEx::of(paddr, sizeof(T)), allocation);
        }

        template<typename T> requires (std::is_standard_layout_v<T>)
        [[nodiscard]]
        T *mapMmio(PhysicalAddressEx paddr, PageFlags flags, TlsfAllocation *allocation [[gnu::nonnull]]) {
            return mapObject<T>(MemoryRangeEx::of(paddr, sizeof(T)), flags, MemoryType::eUncached, allocation);
        }

        template<typename T> requires (std::is_standard_layout_v<T>)
        [[nodiscard]]
        T *mapMmio(MemoryRangeEx range, PageFlags flags, TlsfAllocation *allocation [[gnu::nonnull]]) {
            return mapObject<T>(range, flags, MemoryType::eUncached, allocation);
        }

        void updateHigherHalfMappings(const PageTables *source);

        void updateHigherHalfMappings(const AddressSpace *source) {
            updateHigherHalfMappings(&source->mTables);
        }

        [[nodiscard]]
        PageTables *tables() { return &mTables; }

        [[nodiscard]]
        const PageTables *tables() const { return &mTables; }

        [[nodiscard]]
        auto pageManager(this auto&& self) { return self.mTables.pageManager(); }

        [[nodiscard]]
        PhysicalAddress getBackingAddress(this auto&& self, const void *ptr) {
            return self.mTables.getBackingAddress(ptr);
        }

        [[nodiscard]]
        PhysicalAddress root() const { return mTables.root(); }

        [[nodiscard]]
        OsStatus map(MemoryRangeEx memory, PageFlags flags, MemoryType type, TlsfAllocation *allocation [[gnu::nonnull]]);

        [[nodiscard]]
        OsStatus map(TlsfAllocation memory, PageFlags flags, MemoryType type, MappingAllocation *allocation [[gnu::nonnull]]);

        [[nodiscard]]
        OsStatus map(AddressMapping mapping, PageFlags flags, MemoryType type, TlsfAllocation *allocation [[gnu::nonnull]]);

        [[nodiscard]]
        OsStatus mapStack(TlsfAllocation memory, PageFlags flags, StackMappingAllocation *allocation [[gnu::nonnull]]);

        [[nodiscard]]
        OsStatus mapStack(MemoryRangeEx memory, PageFlags flags, std::array<TlsfAllocation, 3> *allocations [[gnu::nonnull]]);

        [[nodiscard]]
        OsStatus reserve(size_t size, TlsfAllocation *result [[gnu::nonnull]]);

        [[nodiscard]]
        OsStatus reserve(VirtualRangeEx range, TlsfAllocation *result [[gnu::nonnull]]);

        void release(TlsfAllocation allocation) noexcept;

        [[nodiscard]]
        OsStatus unmap(MappingAllocation allocation);

        [[nodiscard]]
        OsStatus unmap(TlsfAllocation allocation);

        AddressSpaceStats stats() noexcept {
            stdx::LockGuard guard(mLock);
            return {
                .heap = mVmemHeap.stats(),
            };
        }

        [[nodiscard]]
        static OsStatus create(const PageBuilder *pm [[gnu::nonnull]], AddressMapping pteMemory, PageFlags flags, VirtualRangeEx vmem, AddressSpace *space [[gnu::nonnull]]);

        [[nodiscard]]
        static OsStatus create(const AddressSpace *source [[gnu::nonnull]], AddressMapping pteMemory, PageFlags flags, VirtualRangeEx vmem, AddressSpace *space [[gnu::nonnull]]);

#pragma endregion
    };
}
