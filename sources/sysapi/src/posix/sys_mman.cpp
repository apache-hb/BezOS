#include <posix/sys/mman.h>

#include <bezos/facility/vmem.h>

#include "private.hpp"

static OsMemoryAccess ConvertMmapAccess(int prot) {
    OsMemoryAccess access = eOsMemoryReserve | eOsMemoryCommit;

    if (prot & PROT_READ) {
        access |= eOsMemoryRead;
    }

    if (prot & PROT_WRITE) {
        access |= eOsMemoryWrite;
    }

    if (prot & PROT_EXEC) {
        access |= eOsMemoryExecute;
    }

    return access;
}

void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off) {
    if (len == 0) {
        return MAP_FAILED;
    }

    if (fd != -1) {
        Unimplemented(ENOSYS, "mmap(fd != -1)");
        return MAP_FAILED;
    }

    if (flags & MAP_SHARED) {
        Unimplemented(ENOSYS, "mmap MAP_SHARED");
        return MAP_FAILED;
    }

    OsMemoryAccess access = ConvertMmapAccess(prot);
    if ((flags & MAP_FIXED) == 0) {
        access |= eOsMemoryAddressHint;
    }

    if ((flags & MAP_UNINITIALIZED) == 0) {
        access |= eOsMemoryDiscard;
    }

    OsVmemCreateInfo createInfo {
        .BaseAddress = addr,
        .Size = len,
        .Alignment = 0,
        .Access = access,
    };

    OsAnyPointer result = nullptr;
    OsStatus status = OsVmemCreate(createInfo, &result);
    if (status == OsStatusSuccess) {
        return result;
    }

    switch (status) {
    case OsStatusOutOfMemory:
        errno = ENOMEM;
        break;
    default:
        errno = EINVAL;
        break;
    }

    return MAP_FAILED;
}

int munmap(void *, size_t) {
    Unimplemented();
    return -1;
}

int posix_madvise(void *, size_t, int) {
    Unimplemented();
    return -1;
}
