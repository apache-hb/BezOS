#include "boot.hpp"

bool boot::MemoryRegion::isUsable() const {
    switch (mType) {
    case MemoryRegionType::eUsable:
    case MemoryRegionType::eAcpiReclaimable:
    case MemoryRegionType::eBootloaderReclaimable:
        return true;

    default:
        return false;
    }
}

bool boot::MemoryRegion::isReclaimable() const {
    switch (mType) {
    case MemoryRegionType::eAcpiReclaimable:
    case MemoryRegionType::eBootloaderReclaimable:
        return true;

    default:
        return false;
    }
}

bool boot::MemoryRegion::isAccessible() const {
    return mType != MemoryRegionType::eBadMemory;
}
