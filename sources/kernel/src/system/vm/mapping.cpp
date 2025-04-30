#include "log.hpp"
#include "memory/address_space.hpp"
#include "memory/page_allocator.hpp"
#include "system/vm/file.hpp"
#include "system/vm/memory.hpp"

OsStatus sys2::MapFileToMemory(sm::RcuDomain *domain, vfs2::IFileHandle *fileHandle, km::PageAllocator *pmm, km::AddressSpace *ptes, uint64_t front, uint64_t back, sm::RcuSharedPtr<FileMapping> *outMapping) {
    uint64_t size = back - front;

    auto memory = pmm->alloc4k(km::Pages(size));
    if (memory.isEmpty()) {
        return OsStatusOutOfMemory;
    }

    km::AddressMapping mapping;
    if (OsStatus status = ptes->map(memory, km::PageFlags::eData, km::MemoryType::eWriteBack, &mapping)) {
        pmm->release(memory);
        return status;
    }

    KmDebugMessage("[VMEM] Creating mapping: ", mapping, "\n");

    vfs2::ReadRequest read {
        .begin = (char*)mapping.vaddr,
        .end = (char*)mapping.vaddr + size,
        .offset = front,
    };
    vfs2::ReadResult result {};
    if (OsStatus status = fileHandle->read(read, &result)) {
        KmDebugMessage("[VMEM] Failed to read file mapping from ", front, " to ", back, ": ", OsStatusId(status), "\n");
        pmm->release(memory);
        (void)ptes->unmap(mapping.virtualRange());
        return status;
    }

    if (result.read != size) {
        KmDebugMessage("[VMEM] Mapping could not be fully read: ", result.read, " != ", size, "\n");
        pmm->release(memory);
        (void)ptes->unmap(mapping.virtualRange());
        return OsStatusInvalidData;
    }

    auto object = sm::rcuMakeShared<FileMapping>(domain, mapping);
    if (!object) {
        pmm->release(memory);
        (void)ptes->unmap(mapping.virtualRange());
        return OsStatusOutOfMemory;
    }

    *outMapping = object;
    return OsStatusSuccess;
}

OsStatus sys2::NewMemoryObject(sm::RcuDomain *domain, km::MemoryRange range, sm::RcuSharedPtr<MemoryObject> *outObject) {
    auto object = sm::rcuMakeShared<MemoryObject>(domain, range);
    if (!object) {
        return OsStatusOutOfMemory;
    }

    *outObject = object;
    return OsStatusSuccess;
}
