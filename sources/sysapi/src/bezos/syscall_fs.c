#include <bezos/facility/fs.h>

#include <bezos/private.h>

//
// File related syscalls
//

OsStatus OsFileOpen(struct OsFileCreateInfo CreateInfo, OsFileHandle *OutHandle) {
    struct OsCallResult result = OsSystemCall(eOsCallFileOpen, (uint64_t)&CreateInfo, 0, 0, 0);
    *OutHandle = (OsFileHandle)result.Value;
    return result.Status;
}

OsStatus OsFileClose(OsFileHandle Handle) {
    struct OsCallResult result = OsSystemCall(eOsCallFileClose, (uint64_t)Handle, 0, 0, 0);
    return result.Status;
}

OsStatus OsFileRead(OsFileHandle Handle, void *BufferFront, void *BufferBack, uint64_t *OutRead) {
    struct OsCallResult result = OsSystemCall(eOsCallFileRead, (uint64_t)Handle, (uint64_t)BufferFront, (uint64_t)BufferBack, 0);
    *OutRead = (uint64_t)result.Value;
    return result.Status;
}

OsStatus OsFileWrite(OsFileHandle Handle, const void *BufferFront, const void *BufferBack, uint64_t *OutWritten) {
    struct OsCallResult result = OsSystemCall(eOsCallFileWrite, (uint64_t)Handle, (uint64_t)BufferFront, (uint64_t)BufferBack, 0);
    *OutWritten = (uint64_t)result.Value;
    return result.Status;
}

OsStatus OsFileSeek(OsFileHandle Handle, OsSeekMode Mode, int64_t Offset, uint64_t *OutPosition) {
    struct OsCallResult result = OsSystemCall(eOsCallFileSeek, (uint64_t)Handle, (uint64_t)Mode, (uint64_t)Offset, 0);
    *OutPosition = (uint64_t)result.Value;
    return result.Status;
}

OsStatus OsFileStat(OsFileHandle Handle, struct OsFileStat *OutStat) {
    struct OsCallResult result = OsSystemCall(eOsCallFileStat, (uint64_t)Handle, (uint64_t)OutStat, 0, 0);
    return result.Status;
}

//
// Folder related syscalls
//

OsStatus OsFolderIterateCreate(struct OsFolderIterateCreateInfo CreateInfo, OsFolderIteratorHandle *OutHandle) {
    struct OsCallResult result = OsSystemCall(eOsCallFolderIterateCreate, (uint64_t)&CreateInfo, 0, 0, 0);
    *OutHandle = (OsFolderIteratorHandle)result.Value;
    return result.Status;
}

OsStatus OsFolderIterateDestroy(OsFolderIteratorHandle Handle) {
    struct OsCallResult result = OsSystemCall(eOsCallFolderIterateDestroy, (uint64_t)Handle, 0, 0, 0);
    return result.Status;
}

OsStatus OsFolderIterateNext(OsFolderIteratorHandle Handle, struct OsFolderEntry *OutEntry) {
    struct OsCallResult result = OsSystemCall(eOsCallFolderIterateNext, (uint64_t)Handle, (uint64_t)OutEntry, 0, 0);
    return result.Status;
}
