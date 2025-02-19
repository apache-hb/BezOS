#include "boot.hpp"

bool boot::MemoryRegion::isUsable() const {
    switch (type) {
    case eUsable:
    case eAcpiReclaimable:
    case eBootloaderReclaimable:
        return true;

    default:
        return false;
    }
}

bool boot::MemoryRegion::isReclaimable() const {
    switch (type) {
    case eAcpiReclaimable:
    case eBootloaderReclaimable:
        return true;

    default:
        return false;
    }
}

bool boot::MemoryRegion::isAccessible() const {
    return type != eBadMemory;
}

sm::Memory boot::MemoryMap::usableMemory() const {
    size_t result = 0;
    for (const MemoryRegion& entry : regions) {
        if (entry.isUsable()) {
            result += entry.size();
        }
    }
    return sm::bytes(result);
}

sm::Memory boot::MemoryMap::reclaimableMemory() const {
    size_t result = 0;
    for (const MemoryRegion& entry : regions) {
        if (entry.isReclaimable()) {
            result += entry.size();
        }
    }
    return sm::bytes(result);
}

km::PhysicalAddress boot::MemoryMap::maxPhysicalAddress() const {
    km::PhysicalAddress result = nullptr;
    for (const MemoryRegion& entry : regions) {
        if (entry.isAccessible()) {
            result = std::max(result, entry.range.back);
        }
    }
    return result;
}
