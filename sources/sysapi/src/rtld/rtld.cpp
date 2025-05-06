#include "rtld/rtld.h"

#include "rtld/load/elf.hpp"

#include "common/util/util.hpp"
#include "common/util/defer.hpp"

#include <bezos/facility/debug.h>
#include <bezos/facility/fs.h>
#include <bezos/facility/vmem.h>
#include <bezos/subsystem/fs.h>
#include <bezos/facility/process.h>
#include <bezos/facility/threads.h>
#include <bezos/start.h>

#include <span>
#include <stdlib.h>

namespace elf = os::elf;

template<size_t N>
static void DebugLog(const char (&message)[N]) {
    OsDebugMessage(eOsLogDebug, message);
}

template<typename T>
static void DebugLog(T number) {
    char buffer[32];
    char *ptr = buffer + sizeof(buffer);
    *--ptr = '\0';

    if (number == 0) {
        *--ptr = '0';
    } else {
        while (number != 0) {
            *--ptr = '0' + (number % 10);
            number /= 10;
        }
    }

    OsDebugMessage({ ptr, buffer + sizeof(buffer), eOsLogDebug });
}

static const elf::ProgramHeader *FindProgramHeader(std::span<const elf::ProgramHeader> phs, elf::ProgramHeaderType type) {
    for (const auto& header : phs) {
        if (header.type == type) {
            return &header;
        }
    }

    return nullptr;
}

static const elf::ProgramHeader *FindTlsSection(std::span<const elf::ProgramHeader> phs) {
    return FindProgramHeader(phs, elf::ProgramHeaderType::eTls);
}

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

    uint64_t phend = header.phoff + header.phnum * header.phentsize;

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

static OsStatus RtldTlsInit(OsDeviceHandle file, OsProcessHandle process, const elf::ProgramHeader &ph, RtldTlsInitInfo *tlsInfo) {
    if (ph.memsz != ph.filesz && ph.filesz != 0) {
        void *initAddress = nullptr;
        OsVmemCreateInfo vmemGuestCreateInfo {
            .BaseAddress = OsAnyPointer(ph.vaddr),
            .Size = sm::roundup<uint64_t>(ph.memsz, 0x1000),
            .Access = eOsMemoryRead | eOsMemoryCommit | eOsMemoryDiscard,
            .Process = process,
        };

        if (OsStatus status = OsVmemCreate(vmemGuestCreateInfo, &initAddress)) {
            OsDebugMessage(eOsLogInfo, "Failed to create TLS memory");
            return status;
        }
    }

    void *initAddress = nullptr;

    if (ph.filesz != 0) {
        OsVmemMapInfo vmemGuestMapInfo {
            .SrcAddress = ph.offset,
            .DstAddress = ph.vaddr,
            .Size = ph.filesz,
            .Access = eOsMemoryRead,
            .Source = file,
            .Process = process,
        };

        if (OsStatus status = OsVmemMap(vmemGuestMapInfo, &initAddress)) {
            OsDebugMessage(eOsLogInfo, "Failed to map TLS memory");
            return status;
        }
    }

    *tlsInfo = RtldTlsInitInfo {
        .InitAddress = initAddress,
        .TlsDataSize = ph.filesz,
        .TlsBssSize = ph.memsz - ph.filesz,
        .TlsAlign = ph.align,
    };

    DebugLog("TlsAlign:");
    DebugLog(tlsInfo->TlsAlign);
    DebugLog(ph.align);

    return OsStatusSuccess;
}

static OsStatus RtldMapProgram(OsDeviceHandle file, OsProcessHandle process, uintptr_t *entry, RtldTlsInitInfo *tlsInfo) {
    elf::Header header;

    if (OsStatus status = ElfReadHeader(file, &header)) {
        return status;
    }

    if (header.phnum == 0) {
        return OsStatusInvalidData;
    }

    void *elfPhMappingBase = nullptr;
    OsVmemMapInfo mapInfo {
        .SrcAddress = sm::rounddown(header.phoff, UINT64_C(0x1000)),
        .Size = OsSize(header.phnum * header.phentsize),
        .Access = eOsMemoryRead,
        .Source = file,
    };

    if (OsStatus status = OsVmemMap(mapInfo, &elfPhMappingBase)) {
        return status;
    }

    void *elfPhMapping = (void*)((uintptr_t)elfPhMappingBase + (header.phoff % 0x1000));

    defer {
        OsVmemRelease(elfPhMappingBase, OsSize(header.phnum * header.phentsize));
    };

    *entry = header.entry;
    std::span<elf::ProgramHeader> phs(reinterpret_cast<elf::ProgramHeader*>(elfPhMapping), header.phnum);

    DebugLog("Program headers:");
    DebugLog(header.phoff);
    DebugLog(header.phentsize);
    DebugLog(header.phnum);
    DebugLog((uint64_t)elfPhMapping);

    for (const elf::ProgramHeader &ph : phs) {
        DebugLog("Loading program header");
        DebugLog((uint32_t)ph.type);
        DebugLog(ph.flags);

        if (ph.type != elf::ProgramHeaderType::eLoad) {
            continue;
        }

        OsMemoryAccess access = eOsMemoryNoAccess;
        if (ph.flags & (1 << 0))
            access |= eOsMemoryExecute;
        if (ph.flags & (1 << 1))
            access |= eOsMemoryWrite;
        if (ph.flags & (1 << 2))
            access |= eOsMemoryRead;

        if (ph.memsz != ph.filesz) {
            OsVmemCreateInfo vmemGuestCreateInfo {
                .BaseAddress = reinterpret_cast<void*>(ph.vaddr),
                .Size = sm::roundup<uint64_t>(ph.memsz, 0x1000),
                .Access = access | eOsMemoryCommit | eOsMemoryDiscard,
                .Process = process,
            };

            void *guestAddress = nullptr;
            if (OsStatus status = OsVmemCreate(vmemGuestCreateInfo, &guestAddress)) {
                return status;
            }
        }

        OsVmemMapInfo vmemGuestMapInfo {
            .SrcAddress = ph.offset,
            .DstAddress = ph.vaddr,
            .Size = ph.filesz,
            .Access = access,
            .Source = file,
            .Process = process,
        };
        void *guestAddress = nullptr;

        if (OsStatus status = OsVmemMap(vmemGuestMapInfo, &guestAddress)) {
            return status;
        }
    }

    if (const elf::ProgramHeader *tls = FindTlsSection(phs)) {
        if (OsStatus status = RtldTlsInit(file, process, *tls, tlsInfo)) {
            return status;
        }
    }

    return OsStatusSuccess;
}

OsStatus RtldStartProgram(const RtldStartInfo *StartInfo, OsThreadHandle *OutThread) {
    uintptr_t entry = 0;
    RtldTlsInitInfo tlsInfo{};
    if (OsStatus status = RtldMapProgram(StartInfo->Program, StartInfo->Process, &entry, &tlsInfo)) {
        return status;
    }

    uintptr_t startInfoSize = sm::roundup<size_t>(sizeof(OsClientStartInfo), 0x1000);

    void *startInfoGuestAddress = nullptr;
    OsVmemCreateInfo startInfoCreateInfo {
        .Size = startInfoSize,
        .Access = eOsMemoryRead | eOsMemoryDiscard | eOsMemoryCommit,
        .Process = StartInfo->Process,
    };

    if (OsStatus status = OsVmemCreate(startInfoCreateInfo, &startInfoGuestAddress)) {
        return status;
    }

    OsVmemMapInfo startInfoMapInfo {
        .SrcAddress = reinterpret_cast<OsAddress>(startInfoGuestAddress),
        .Size = startInfoSize,
        .Access = eOsMemoryRead | eOsMemoryWrite,
        .Source = StartInfo->Process,
    };

    if (OsStatus status = OsVmemMap(startInfoMapInfo, &startInfoGuestAddress)) {
        return status;
    }

    defer {
        OsVmemRelease(startInfoGuestAddress, startInfoSize);
    };

    OsClientStartInfo *startInfo = reinterpret_cast<OsClientStartInfo*>(startInfoGuestAddress);

    OsVmemCreateInfo stackCreateInfo {
        .Size = 0x4000,
        .Access = eOsMemoryRead | eOsMemoryWrite | eOsMemoryCommit | eOsMemoryDiscard,
        .Process = StartInfo->Process,
    };
    void *stackGuestAddress = nullptr;
    if (OsStatus status = OsVmemCreate(stackCreateInfo, &stackGuestAddress)) {
        return status;
    }

    *startInfo = OsClientStartInfo {
        .TlsInit = tlsInfo.InitAddress,
        .TlsDataSize = tlsInfo.TlsDataSize,
        .TlsBssSize = tlsInfo.TlsBssSize,
    };

    RtldTls tls{};
    DebugLog(tlsInfo.TlsDataSize);
    DebugLog(tlsInfo.TlsBssSize);
    DebugLog((uintptr_t)tlsInfo.InitAddress);
    if (tlsInfo.TlsDataSize != 0 || tlsInfo.TlsBssSize != 0) {
        if (OsStatus status = RtldCreateTls(StartInfo->Process, &tlsInfo, &tls)) {
            return status;
        }
    }

    uintptr_t stackBase = reinterpret_cast<uintptr_t>(stackGuestAddress) + 0x4000 - 0x8;

    OsThreadCreateInfo threadCreateInfo {
        .Name = "ENTRY",
        .CpuState = OsMachineContext {
            .rdi = reinterpret_cast<uintptr_t>(startInfoGuestAddress),
            .rbp = stackBase,
            .rsp = stackBase,
            .rip = entry,
        },
        .TlsAddress = OsAddress(tls.TlsAddress),
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

OsStatus RtldCreateTls(OsProcessHandle Process, struct RtldTlsInitInfo *TlsInfo, RtldTls *OutTlsInfo) {
    DebugLog(TlsInfo->TlsDataSize);
    DebugLog(TlsInfo->TlsBssSize);
    DebugLog(TlsInfo->TlsAlign);
    size_t tlsSize = sm::roundup(TlsInfo->TlsDataSize + TlsInfo->TlsBssSize, TlsInfo->TlsAlign);
    size_t initSize = sm::roundup<size_t>(TlsInfo->TlsDataSize, 0x1000);
    size_t totalSize = sm::roundup<size_t>(tlsSize + sizeof(uintptr_t), 0x1000);

    // Create the TLS area in the guest process.
    OsVmemCreateInfo createInfo {
        .Size = totalSize,
        .Access = eOsMemoryRead | eOsMemoryWrite | eOsMemoryCommit | eOsMemoryDiscard,
        .Process = Process,
    };

    void *guestAddress = nullptr;
    if (OsStatus status = OsVmemCreate(createInfo, &guestAddress)) {
        return status;
    }

    // Map the TLS area into our address space.
    void *tlsAddress = nullptr;
    OsVmemMapInfo mapTlsInfo {
        .SrcAddress = reinterpret_cast<OsAddress>(guestAddress),
        .Size = totalSize,
        .Access = eOsMemoryRead | eOsMemoryWrite,
        .Source = Process,
    };
    if (OsStatus status = OsVmemMap(mapTlsInfo, &tlsAddress)) {
        return status;
    }

    defer {
        OsVmemRelease(tlsAddress, totalSize);
    };

    // Map the TLS init data into our address space.
    if (TlsInfo->InitAddress != nullptr) {
        void *initAddress = nullptr;
        OsVmemMapInfo mapTlsInitInfo {
            .SrcAddress = reinterpret_cast<OsAddress>(TlsInfo->InitAddress),
            .Size = initSize,
            .Access = eOsMemoryRead,
            .Source = Process,
        };
        if (OsStatus status = OsVmemMap(mapTlsInitInfo, &initAddress)) {
            OsDebugMessage(eOsLogInfo, "Failed to map TLS init area");
            return status;
        }

        defer {
            OsVmemRelease(initAddress, initSize);
        };

        // Copy the TLS init data into the TLS area.
        memcpy(tlsAddress, initAddress, TlsInfo->TlsDataSize);
    }

    uintptr_t tlsControlBlock = reinterpret_cast<uintptr_t>(guestAddress) + tlsSize;

    // Copy the self pointer to the end of the TLS area.
    *(uintptr_t*)((char*)tlsAddress + tlsSize) = tlsControlBlock;

    *OutTlsInfo = RtldTls {
        .BaseAddress = guestAddress,
        .TlsAddress = reinterpret_cast<void*>(tlsControlBlock),
        .Size = totalSize,
    };

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