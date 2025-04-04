#include <bezos/facility/vmem.h>

#include <bezos/private.h>

OsStatus OsVmemCreate(struct OsVmemCreateInfo CreateInfo, OsAnyPointer *OutBase) {
    struct OsCallResult result = OsSystemCall(eOsCallVmemCreate, (uint64_t)&CreateInfo, 0, 0, 0);
    *OutBase = (OsAnyPointer)result.Value;
    return result.Status;
}

OsStatus OsVmemMap(struct OsVmemMapInfo MapInfo, OsAnyPointer *OutBase) {
    struct OsCallResult result = OsSystemCall(eOsCallVmemMap, (uint64_t)&MapInfo, 0, 0, 0);
    *OutBase = (OsAnyPointer)result.Value;
    return result.Status;
}

OsStatus OsVmemRelease(OsAnyPointer BaseAddress, OsSize Size) {
    struct OsCallResult result = OsSystemCall(eOsCallVmemRelease, (uint64_t)BaseAddress, Size, 0, 0);
    return result.Status;
}
