#pragma once

#include "std/std.hpp"

#include "memory/memory.hpp"
#include "memory/paging.hpp"

namespace sys {
    class MemoryManager;

    /// @brief A request to map physical memory into an address space.
    class AddressSpaceMappingRequest {
        MemoryManager *mMemory;
        sm::PhysicalAddress mAddress;
        sm::VirtualAddress mVirtual;
        size_t mSize;
        size_t mAlignment;
        km::PageFlags mFlags;
        km::MemoryType mType;
        bool mAddressIsHint;

    public:
        MemoryManager *getMemory() const noexcept;
        size_t getSize() const noexcept;
        size_t getAlignment() const noexcept;
        sm::PhysicalAddress getPhysicalAddress() const noexcept;
        sm::VirtualAddress getVirtualAddress() const noexcept;
        km::PageFlags getPageFlags() const noexcept;
        km::MemoryType getMemoryType() const noexcept;
        bool isAddressHint() const noexcept;

        AddressSpaceMappingRequest(MemoryManager *memory [[gnu::nonnull]], sm::PhysicalAddress address, sm::VirtualAddress virtualAddress, size_t size, size_t align, km::PageFlags flags, km::MemoryType type, bool addressIsHint) noexcept;

        /// @brief Create a new address space mapping request.
        ///
        /// @pre @p memory is not null.
        /// @pre @p size is not zero, and is a multiple of @a x64::kPageSize.
        /// @pre @p align is a power of two, and is at least @a x64::kPageSize.
        /// @pre @p address is either null or aligned to @p align.
        /// @pre @p result is not null.
        ///
        /// @param memory The memory manager to allocate physical memory from.
        /// @param address The physical address to map.
        /// @param virtualAddress The virtual address to map the memory at, or null to allocate anywhere.
        /// @param size The size of the mapping.
        /// @param align The alignment of the mapping.
        /// @param flags The page flags to use for the mapping.
        /// @param type The memory type to use for the mapping.
        /// @param addressIsHint If true, the virtual address is a hint and may be ignored.
        /// @param result The resulting mapping request.
        ///
        /// @return The status of the operation.
        ///
        /// @retval OsStatusSuccess The request was created successfully.
        /// @retval OsStatusInvalidArgument One or more arguments were invalid.
        [[nodiscard]]
        static OsStatus create(MemoryManager *memory [[gnu::nonnull]], sm::PhysicalAddress address, sm::VirtualAddress virtualAddress, size_t size, size_t align, km::PageFlags flags, km::MemoryType type, bool addressIsHint, AddressSpaceMappingRequest *result [[outparam]]) noexcept;

        class Builder {
            MemoryManager *mMemory{};
            sm::PhysicalAddress mAddress{};
            sm::VirtualAddress mVirtual{};
            size_t mSize{};
            size_t mAlignment{};
            km::PageFlags mFlags{};
            km::MemoryType mType = km::MemoryType::eWriteBack;
            bool mAddressIsHint = false;

        public:
            Builder& memory(MemoryManager *memory [[gnu::nonnull]]) noexcept;
            Builder& physicalAddress(sm::PhysicalAddress address) noexcept;
            Builder& virtualAddress(sm::VirtualAddress address) noexcept;
            Builder& size(size_t size) noexcept;
            Builder& alignment(size_t align) noexcept;
            Builder& flags(km::PageFlags flags) noexcept;
            Builder& type(km::MemoryType type) noexcept;
            Builder& addressIsHint(bool hint) noexcept;

            [[nodiscard]]
            AddressSpaceMappingRequest build() const noexcept;
        };

        static Builder builder() noexcept {
            return Builder {};
        }
    };

    km::AddressMapping getMapping(const AddressSpaceMappingRequest& request) noexcept;
}
