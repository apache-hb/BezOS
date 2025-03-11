#include <bezos/facility/vmem.h>

#include <bezos/private.h>

OsStatus OsVmemCreate(struct OsVmemCreateInfo CreateInfo, OsAnyPointer *OutBase) {
    struct OsCallResult result = OsSystemCall(eOsCallVmemCreate, (uint64_t)&CreateInfo, 0, 0, 0);
    *OutBase = (OsAnyPointer)result.Value;
    return result.Status;
}
