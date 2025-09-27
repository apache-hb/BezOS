#pragma once

#include "std/std.hpp"

#include "memory/layout.hpp"
#include "memory/memory.hpp"
#include "memory/paging.hpp"

namespace km {
    /// @brief A request to map a range of physical memory to an area of virtual address space.
    class PageMappingRequest {
        AddressMapping mMapping{};
        PageFlags mFlags{};
        MemoryType mType{};

    public:
        PageMappingRequest() noexcept = default;

        /// @brief Construct a page mapping request.
        /// @see PageMappingRequest#PageMappingRequest(AddressMapping, PageFlags, MemoryType)
        PageMappingRequest(AddressMapping mapping, PageFlags flags) noexcept;

        /// @brief Construct a page mapping request.
        ///
        /// @pre @p mapping.size must be non-zero and a multiple of @a x64::kPageSize.
        /// @pre @p mapping.vaddr must be aligned to @a x64::kPageSize.
        /// @pre @p mapping.paddr must be aligned to @a x64::kPageSize.
        /// @pre @p mapping.vaddr must be a canonical address.
        /// @pre @p flags must not be @c PageFlags::eNone.
        ///
        /// @param mapping The mapping to create.
        /// @param flags The page flags to use for the mapping.
        /// @param type The memory type to use for the mapping.
        ///
        /// @return The page mapping request.
        PageMappingRequest(AddressMapping mapping, PageFlags flags, MemoryType type) noexcept;

        /// @brief Get the mapping for this request.
        ///
        /// @return The address mapping.
        AddressMapping mapping() const noexcept [[clang::nonblocking]];

        /// @brief Get the flags for this request.
        ///
        /// @return The page flags.
        PageFlags flags() const noexcept [[clang::nonblocking]];

        /// @brief Get the memory type for this request.
        ///
        /// @return The memory type.
        MemoryType type() const noexcept [[clang::nonblocking]];

        /// @brief Create a page mapping request.
        ///
        /// Unlink the constructor, this function returns an error code instead of panicking.
        ///
        /// @see PageMappingRequest#PageMappingRequest(AddressMapping, PageFlags, MemoryType)
        ///
        /// @param mapping The mapping to create.
        /// @param flags The page flags to use for the mapping.
        /// @param type The memory type to use for the mapping.
        /// @param request The newly created request.
        ///
        /// @return The status of the operation.
        ///
        /// @retval OsStatusSuccess The request was created successfully, @p request is initialized.
        /// @retval OsStatusInvalidInput The input parameters were malformed, no request has been created.
        [[nodiscard]]
        static OsStatus create(AddressMapping mapping, PageFlags flags, MemoryType type, PageMappingRequest *request [[outparam]]) noexcept;
    };
}
