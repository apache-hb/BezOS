#include "system/vmm_map_request.hpp"
#include "logger/categories.hpp"

using sys::AddressSpaceMappingRequest;
using Builder = sys::AddressSpaceMappingRequest::Builder;

static bool isAddressSpaceMappingRequestInputValid(sys::MemoryManager *memory, sm::PhysicalAddress physicalAddress, sm::VirtualAddress virtualAddress, size_t size, size_t align, bool addressIsHint) noexcept {
    if (memory == nullptr) {
        return false;
    }

    if (size == 0 || !sm::isAlignedTo(size, x64::kPageSize)) {
        return false;
    }

    if (align == 0 || !sm::isPowerOf2(align) || align < x64::kPageSize) {
        return false;
    }

    if (physicalAddress.isNull() || !physicalAddress.isAlignedTo(x64::kPageSize)) {
        return false;
    }

    if (addressIsHint) {
        if (virtualAddress.isNull()) {
            return false;
        }
    } else {
        size_t actualAlign = align == 0 ? x64::kPageSize : align;

        if (!virtualAddress.isNull() && !virtualAddress.isAlignedTo(actualAlign)) {
            return false;
        }
    }

    return true;
}

AddressSpaceMappingRequest::AddressSpaceMappingRequest(MemoryManager *memory [[gnu::nonnull]], sm::PhysicalAddress address, sm::VirtualAddress virtualAddress, size_t size, size_t align, km::PageFlags flags, km::MemoryType type, bool addressIsHint) noexcept
    : mMemory(memory)
    , mAddress(address)
    , mVirtual(virtualAddress)
    , mSize(size)
    , mAlignment(align)
    , mFlags(flags)
    , mType(type)
    , mAddressIsHint(addressIsHint)
{
    if (!isAddressSpaceMappingRequestInputValid(memory, address, virtualAddress, size, align, addressIsHint)) {
        MemLog.fatalf("Invalid AddressSpaceMappingRequest parameters: (memory: ", (void*)memory, ", physicalAddress: ", address, ", virtualAddress: ", virtualAddress, ", size: ", size, ", alignment: ", align, ", addressIsHint: ", addressIsHint, ")");
        KM_PANIC("Invalid AddressSpaceMappingRequest parameters");
    }
}

[[nodiscard]]
OsStatus AddressSpaceMappingRequest::create(MemoryManager *memory [[gnu::nonnull]], sm::PhysicalAddress address, sm::VirtualAddress virtualAddress, size_t size, size_t align, km::PageFlags flags, km::MemoryType type, bool addressIsHint, AddressSpaceMappingRequest *result [[outparam]]) noexcept {
    if (!isAddressSpaceMappingRequestInputValid(memory, address, virtualAddress, size, align, addressIsHint)) {
        return OsStatusInvalidInput;
    }

    *result = AddressSpaceMappingRequest{memory, address, virtualAddress, size, align, flags, type, addressIsHint};
    return OsStatusSuccess;
}

sys::MemoryManager *AddressSpaceMappingRequest::getMemory() const noexcept {
    return mMemory;
}

size_t AddressSpaceMappingRequest::getAlignment() const noexcept {
    return mAlignment;
}

size_t AddressSpaceMappingRequest::getSize() const noexcept {
    return mSize;
}

sm::VirtualAddress AddressSpaceMappingRequest::getVirtualAddress() const noexcept {
    return mVirtual;
}

sm::PhysicalAddress AddressSpaceMappingRequest::getPhysicalAddress() const noexcept {
    return mAddress;
}

km::PageFlags AddressSpaceMappingRequest::getPageFlags() const noexcept {
    return mFlags;
}

km::MemoryType AddressSpaceMappingRequest::getMemoryType() const noexcept {
    return mType;
}

bool AddressSpaceMappingRequest::isAddressHint() const noexcept {
    return mAddressIsHint;
}

Builder& Builder::memory(sys::MemoryManager *memory [[gnu::nonnull]]) noexcept {
    mMemory = memory;
    return *this;
}

Builder& Builder::physicalAddress(sm::PhysicalAddress address) noexcept {
    mAddress = address;
    return *this;
}

Builder& Builder::virtualAddress(sm::VirtualAddress address) noexcept {
    mVirtual = address;
    return *this;
}

Builder& Builder::size(size_t size) noexcept {
    mSize = size;
    return *this;
}

Builder& Builder::alignment(size_t align) noexcept {
    mAlignment = align;
    return *this;
}

Builder& Builder::flags(km::PageFlags flags) noexcept {
    mFlags = flags;
    return *this;
}

Builder& Builder::type(km::MemoryType type) noexcept {
    mType = type;
    return *this;
}

Builder& Builder::addressIsHint(bool hint) noexcept {
    mAddressIsHint = hint;
    return *this;
}

[[nodiscard]]
AddressSpaceMappingRequest Builder::build() const noexcept {
    return AddressSpaceMappingRequest { mMemory, mAddress, mVirtual, mSize, mAlignment, mFlags, mType, mAddressIsHint };
}
