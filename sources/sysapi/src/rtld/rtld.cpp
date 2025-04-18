#include "rtld/rtld.h"

#include "rtld/load/elf.hpp"

#include "common/util/util.hpp"
#include "common/util/defer.hpp"

#include <bezos/facility/fs.h>
#include <bezos/facility/vmem.h>
#include <bezos/subsystem/fs.h>
#include <bezos/facility/process.h>
#include <bezos/facility/threads.h>
#include <bezos/start.h>

#include <span>
#include <stdlib.h>

namespace elf = os::elf;

extern "C" char __etext_start[];
extern "C" char __etext_end[];
extern "C" char __edata_start[];
extern "C" char __edata_end[];

template<typename T>
static OsStatus DeviceRead(OsDeviceHandle device, OsSize offset, T *result) {
    OsSize size = 0;
    OsDeviceReadRequest read {
        .BufferFront = reinterpret_cast<char*>(result),
        .BufferBack = reinterpret_cast<char*>(result) + sizeof(T),
        .Offset = offset,
        .Timeout = OS_TIMEOUT_INFINITE,
    };

    if (OsStatus status = OsDeviceRead(device, read, &size)) {
        return status;
    }

    if (size != sizeof(T)) {
        return OsStatusInvalidData;
    }

    return OsStatusSuccess;
}

template<typename T>
static OsStatus DeviceRead(OsDeviceHandle device, OsSize offset, OsSize count, T *result) {
    OsSize size = 0;
    OsDeviceReadRequest read {
        .BufferFront = reinterpret_cast<char*>(result),
        .BufferBack = reinterpret_cast<char*>(result) + sizeof(T) * count,
        .Offset = offset,
        .Timeout = OS_TIMEOUT_INFINITE,
    };

    if (OsStatus status = OsDeviceRead(device, read, &size)) {
        return status;
    }

    if (size != sizeof(T) * count) {
        return OsStatusInvalidData;
    }

    return OsStatusSuccess;
}

static OsStatus ValidateElfHeader(const elf::Header &header, size_t size) {
    if (!header.isValid()) {
        return OsStatusInvalidData;
    }

    if (header.endian() != std::endian::little || header.elfClass() != elf::Class::eClass64 || header.objVersion() != 1) {
        return OsStatusInvalidVersion;
    }

    if (header.type != elf::Type::eExecutable && header.type != elf::Type::eShared) {
        return OsStatusInvalidData;
    }

    if (header.phentsize != sizeof(elf::ProgramHeader)) {
        return OsStatusInvalidData;
    }

    uint64_t phbegin = header.phoff;
    uint64_t phend = phbegin + header.phnum * header.phentsize;

    if (phend > size) {
        return OsStatusInvalidData;
    }

    if (header.entry == 0) {
        return OsStatusInvalidData;
    }

    return OsStatusSuccess;
}

static OsStatus ElfReadHeader(OsDeviceHandle device, elf::Header *result) {
    OsFileInfo info{};

    if (OsStatus status = OsInvokeFileStat(device, &info)) {
        return status;
    }

    elf::Header header;
    if (OsStatus status = DeviceRead(device, 0, &header)) {
        return status;
    }

    if (OsStatus status = ValidateElfHeader(header, info.LogicalSize)) {
        return status;
    }

    *result = header;
    return OsStatusSuccess;
}

static OsStatus RtldMapProgram(OsDeviceHandle file, OsProcessHandle process, uintptr_t *entry) {
    elf::Header header;
    OsStatus status = OsStatusSuccess;

    if ((status = ElfReadHeader(file, &header))) {
        return status;
    }

    if (header.phnum == 0) {
        return OsStatusInvalidData;
    }

    void *elfPhMapping = nullptr;
    OsVmemMapInfo mapInfo {
        .SrcAddress = header.phoff,
        .Size = OsSize(header.phnum * header.phentsize),
        .Access = eOsMemoryRead,
        .Source = file,
    };

    if (OsStatus status = OsVmemMap(mapInfo, &elfPhMapping)) {
        return status;
    }

    defer {
        OsVmemRelease(elfPhMapping, OsSize(header.phnum * header.phentsize));
    };

    *entry = header.entry;
    std::span<elf::ProgramHeader> phs(reinterpret_cast<elf::ProgramHeader*>(elfPhMapping), header.phnum);

    for (elf::ProgramHeader ph : phs) {
        if (ph.type != elf::ProgramHeaderType::eLoad) {
            continue;
        }

        OsMemoryAccess access = eOsMemoryCommit | eOsMemoryPrivate;
        if (ph.flags & (1 << 0))
            access |= eOsMemoryExecute;
        if (ph.flags & (1 << 1))
            access |= eOsMemoryWrite;
        if (ph.flags & (1 << 2))
            access |= eOsMemoryRead;

        OsVmemCreateInfo vmemGuestCreateInfo {
            .BaseAddress = reinterpret_cast<void*>(ph.vaddr),
            .Size = sm::roundup<uint64_t>(ph.memsz, 0x1000),
            .Access = access | eOsMemoryDiscard,
            .Process = process,
        };

        void *guestAddress = nullptr;
        if (OsStatus status = OsVmemCreate(vmemGuestCreateInfo, &guestAddress)) {
            return status;
        }

        OsVmemMapInfo vmemGuestMapInfo {
            .SrcAddress = ph.offset,
            .DstAddress = ph.vaddr,
            .Size = ph.filesz,
            .Access = access,
            .Source = file,
            .Process = process,
        };

        if (OsStatus status = OsVmemMap(vmemGuestMapInfo, &guestAddress)) {
            return status;
        }
    }

    return OsStatusSuccess;
}

OsStatus RtldStartProgram(const RtldStartInfo *StartInfo, OsThreadHandle *OutThread) {
    uintptr_t entry = 0;
    if (OsStatus status = RtldMapProgram(StartInfo->Program, StartInfo->Process, &entry)) {
        return status;
    }

    void *startInfoGuestAddress = nullptr;
    OsVmemCreateInfo startInfoCreateInfo {
        .Size = sizeof(OsClientStartInfo),
        .Access = eOsMemoryRead,
        .Process = StartInfo->Process,
    };

    if (OsStatus status = OsVmemCreate(startInfoCreateInfo, &startInfoGuestAddress)) {
        return status;
    }

    uintptr_t startInfoSize = sm::roundup<size_t>(sizeof(OsClientStartInfo), 0x1000);

    OsVmemMapInfo startInfoMapInfo {
        .SrcAddress = reinterpret_cast<OsAddress>(startInfoGuestAddress),
        .Size = startInfoSize,
        .Access = eOsMemoryRead | eOsMemoryWrite,
        .Source = StartInfo->Program,
    };

    if (OsStatus status = OsVmemMap(startInfoMapInfo, &startInfoGuestAddress)) {
        return status;
    }

    defer {
        OsVmemRelease(startInfoGuestAddress, startInfoSize);
    };

    OsClientStartInfo *startInfo = reinterpret_cast<OsClientStartInfo*>(startInfoGuestAddress);
    *startInfo = OsClientStartInfo {
        .TlsInit = nullptr,
        .TlsInitSize = 0,
    };

    OsVmemCreateInfo stackCreateInfo {
        .Size = 0x4000,
        .Access = eOsMemoryRead | eOsMemoryWrite | eOsMemoryDiscard,
        .Process = StartInfo->Process,
    };
    void *stackGuestAddress = nullptr;
    if (OsStatus status = OsVmemCreate(stackCreateInfo, &stackGuestAddress)) {
        return status;
    }

    uintptr_t stackBase = reinterpret_cast<uintptr_t>(stackGuestAddress) + 0x4000;

    OsThreadCreateInfo threadCreateInfo {
        .Name = "ENTRY",
        .CpuState = OsMachineContext {
            .rdi = reinterpret_cast<uintptr_t>(startInfoGuestAddress),
            .rbp = stackBase,
            .rsp = stackBase,
            .rip = entry,
        },
        .Flags = eOsThreadSuspended,
        .Process = StartInfo->Process,
    };

    OsThreadHandle threadHandle = OS_HANDLE_INVALID;
    if (OsStatus status = OsThreadCreate(threadCreateInfo, &threadHandle)) {
        return status;
    }

    *OutThread = threadHandle;
    return OsStatusSuccess;
}

#if 0
[[maybe_unused]]
static os::IRelocator *GetRelocator(os::elf::Machine machine) {
    if (machine == os::elf::Machine::eAmd64) {
        static os::Amd64Relocator sInstance;
        return &sInstance;
    } else if (machine == os::elf::Machine::eSparcV9) {
        static os::SparcV9Relocator sInstance;
        return &sInstance;
    } else {
        return nullptr;
    }
}

OsStatus RtldSoOpen(const RtldSoLoadInfo *LoadInfo, RtldSo *OutObject) {
    elf::Header header;
    UniquePtr<elf::ProgramHeader[]> phs { nullptr, &free };
    OsStatus status = OsStatusSuccess;

    if ((status = ElfReadHeader(LoadInfo->Object, elf::Type::eShared, &header))) {
        return status;
    }

    if (header.phnum == 0) {
        return OsStatusInvalidData;
    }

    phs = UniqueArray<elf::ProgramHeader>(header.phnum);
    if (phs == nullptr) {
        status = OsStatusOutOfMemory;
        goto error;
    }

error:
    return status;
}

OsStatus RtldSoClose(RtldSo *Object) {
    return OsStatusSuccess;
}

OsStatus RtldSoSymbol(RtldSo *Object, RtldSoName Name, void **OutAddress) {
    return OsStatusNotFound;
}

OsStatus RtldStartProgram(const RtldStartInfo *StartInfo) {
    elf::Header header;
    OsStatus status = OsStatusSuccess;

    if ((status = ElfReadHeader(StartInfo->Program, elf::Type::eExecutable, &header))) {
        return status;
    }

    if (header.phnum == 0) {
        return OsStatusInvalidData;
    }

error:
    return status;
}
#endif

OsStatus RtldSoOpen(const RtldSoLoadInfo *LoadInfo, RtldSo *OutObject) {
    elf::Header header;
    OsStatus status = OsStatusSuccess;

    if ((status = ElfReadHeader(LoadInfo->Object, &header))) {
        return status;
    }

    if (header.phnum == 0) {
        return OsStatusInvalidData;
    }

    return status;
}

OsStatus RtldSoClose(RtldSo *Object) {
    return OsStatusSuccess;
}

OsStatus RtldSoSymbol(RtldSo *Object, RtldSoName Name, void **OutAddress) {
    return OsStatusNotFound;
}