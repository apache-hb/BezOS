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
