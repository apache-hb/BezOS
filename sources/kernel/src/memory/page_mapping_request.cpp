#include "memory/page_mapping_request.hpp"
#include "logger/categories.hpp"

using km::PageMappingRequest;

static bool isPageMappingRequestInputValid(km::AddressMapping mapping, km::PageFlags flags) noexcept [[clang::nonblocking]] {
    if (mapping.size == 0) return false;
    if (!sm::isAlignedTo(mapping.size, x64::kPageSize)) return false;
    if (!sm::isAlignedTo((uintptr_t)mapping.vaddr, x64::kPageSize)) return false;
    if (!sm::isAlignedTo(mapping.paddr.address, x64::kPageSize)) return false;
    if (flags == km::PageFlags::eNone) return false;

    return true;
}

km::PageMappingRequest::PageMappingRequest(AddressMapping mapping, PageFlags flags) noexcept
    : PageMappingRequest(mapping, flags, MemoryType::eWriteBack)
{ }

km::PageMappingRequest::PageMappingRequest(AddressMapping mapping, PageFlags flags, MemoryType type) noexcept
    : mMapping(mapping)
    , mFlags(flags)
    , mType(type)
{
    if (!isPageMappingRequestInputValid(mapping, flags)) {
        MemLog.fatalf("Invalid page mapping request parameters: (mapping: ", mapping, ", flags: ", flags, ", type: ", type, ")");
        KM_PANIC("Invalid page mapping request parameters");
    }
}

km::AddressMapping PageMappingRequest::mapping() const noexcept [[clang::nonblocking]] {
    return mMapping;
}

km::PageFlags PageMappingRequest::flags() const noexcept [[clang::nonblocking]] {
    return mFlags;
}

km::MemoryType PageMappingRequest::type() const noexcept [[clang::nonblocking]] {
    return mType;
}

OsStatus PageMappingRequest::create(AddressMapping mapping, PageFlags flags, MemoryType type, PageMappingRequest *request [[outparam]]) noexcept {
    if (!isPageMappingRequestInputValid(mapping, flags)) {
        return OsStatusInvalidInput;
    }

    *request = PageMappingRequest(mapping, flags, type);
    return OsStatusSuccess;
}
