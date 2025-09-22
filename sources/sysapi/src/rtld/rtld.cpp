#include "rtld/rtld.h"

#include "rtld/load/elf.hpp"

#include "common/util/util.hpp"
#include "common/util/defer.hpp"

#include <bezos/facility/debug.h>
#include <bezos/facility/node.h>
#include <bezos/facility/vmem.h>
#include <bezos/facility/process.h>
#include <bezos/facility/thread.h>
#include <bezos/subsystem/fs.h>
#include <bezos/start.h>

#include <span>
#include <stdlib.h>

#define STB_SPRINTF_IMPLEMENTATION 1
#define STB_SPRINTF_STATIC 1
#include "stb_sprintf.h"

namespace elf = os::elf;

static void RtldPrintf(const char *fmt, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    int size = stbsp_vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    OsDebugMessageInfo messageInfo {
        .Front = buffer,
        .Back = buffer + size,
        .Info = eOsLogDebug,
    };
    OsDebugMessage(messageInfo);
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

#if 0
static OsStatus RemoteMemCopy(void *dst, const void *src, size_t size, OsProcessHandle process) {
    OsVmemMapInfo mapSourceInfo {
        .SrcAddress = OsAddress(src),
        .Size = size,
        .Access = eOsMemoryRead,
        .Source = process,
    };

    OsVmemMapInfo mapDestinationInfo {
        .SrcAddress = OsAddress(dst),
        .Size = size,
        .Access = eOsMemoryWrite,
        .Source = process,
    };

    void *sourceMapping = nullptr;
    if (OsStatus status = OsVmemMap(mapSourceInfo, &sourceMapping)) {
        RtldPrintf("Failed to map source memory for remote copy: ", OsStatusId(status));
        return status;
    }

    void *destinationMapping = nullptr;
    if (OsStatus status = OsVmemMap(mapDestinationInfo, &destinationMapping)) {
        RtldPrintf("Failed to map destination memory for remote copy: ", OsStatusId(status));
        OsVmemRelease(sourceMapping, size);
        return status;
    }

    memcpy(destinationMapping, sourceMapping, size);

    OsVmemRelease(sourceMapping, size);
    OsVmemRelease(destinationMapping, size);

    return OsStatusSuccess;
}
#endif

static OsStatus RtldTlsInit(OsDeviceHandle file, OsProcessHandle process, const elf::ProgramHeader &ph, RtldTlsInitInfo *tlsInfo) {
    void *initAddress = (void*)ph.vaddr;
#if 0
    if (ph.memsz != ph.filesz && ph.filesz != 0) {
        OsVmemCreateInfo vmemGuestCreateInfo {
            .BaseAddress = OsAnyPointer(ph.vaddr),
            .Size = sm::roundup<uint64_t>(ph.memsz, 0x1000),
            .Access = eOsMemoryRead | eOsMemoryCommit | eOsMemoryDiscard,
            .Process = process,
        };

        if (OsStatus status = OsVmemCreate(vmemGuestCreateInfo, &initAddress)) {
            OsDebugMessage(eOsLogInfo, "Failed to create TLS init memory");
            return status;
        }
    }

    if (ph.filesz != 0) {
        OsVmemMapInfo vmemGuestMapInfo {
            .SrcAddress = ph.offset,
            .DstAddress = OsAddress(ph.vaddr),
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
#endif
    *tlsInfo = RtldTlsInitInfo {
        .InitAddress = initAddress,
        .TlsDataSize = ph.filesz,
        .TlsBssSize = ph.memsz - ph.filesz,
        .TlsAlign = ph.align,
    };

    return OsStatusSuccess;
}

static OsStatus RtldGetMemoryRange(std::span<const elf::ProgramHeader> phs, size_t *outBegin, size_t *outEnd) {
    uintptr_t minAddr = UINTPTR_MAX;
    uintptr_t maxAddr = 0;

    for (const elf::ProgramHeader &ph : phs) {
        if (ph.type != elf::ProgramHeaderType::eLoad) {
            continue;
        }

        if (ph.vaddr < minAddr) {
            minAddr = ph.vaddr;
        }

        if (ph.vaddr + ph.memsz > maxAddr) {
            maxAddr = ph.vaddr + ph.memsz;
        }
    }

    if (minAddr == UINTPTR_MAX || maxAddr == 0 || maxAddr <= minAddr) {
        return OsStatusInvalidData;
    }

    *outBegin = sm::rounddown<size_t>(minAddr, 0x1000);
    *outEnd = sm::roundup<size_t>(maxAddr, 0x1000);
    return OsStatusSuccess;
}

static OsStatus RtldMapProgram(OsDeviceHandle file, OsProcessHandle process, uintptr_t *entry, RtldTlsInitInfo *tlsInfo) {
    elf::Header header;

    if (OsStatus status = ElfReadHeader(file, &header)) {
        OsDebugMessage(eOsLogInfo, "Failed to read ELF header");
        return status;
    }

    if (header.phnum == 0) {
        return OsStatusInvalidData;
    }

    void *elfPhMappingBase = nullptr;
    OsVmemMapInfo mapInfo {
        .SrcAddress = sm::rounddown(header.phoff, UINT64_C(0x1000)),
        .Size = sm::roundup(OsSize(header.phnum * header.phentsize) + (header.phoff % 0x1000), UINT64_C(0x1000)),
        .Access = eOsMemoryRead,
        .Source = file,
    };

    if (OsStatus status = OsVmemMap(mapInfo, &elfPhMappingBase)) {
        OsDebugMessage(eOsLogInfo, "Failed to map ELF program headers");
        return status;
    }

    void *elfPhMapping = (void*)((uintptr_t)elfPhMappingBase + (header.phoff % 0x1000));

    defer {
        OsVmemRelease(elfPhMappingBase, OsSize(header.phnum * header.phentsize));
    };

    *entry = header.entry;
    std::span<elf::ProgramHeader> phs(reinterpret_cast<elf::ProgramHeader*>(elfPhMapping), header.phnum);

    size_t memoryBegin = 0;
    size_t memoryEnd = 0;
    if (OsStatus status = RtldGetMemoryRange(phs, &memoryBegin, &memoryEnd)) {
        OsDebugMessage(eOsLogInfo, "Failed to get memory range");
        return status;
    }

    RtldPrintf("Allocating memory: %p-%p", (void*)memoryBegin, (void*)memoryEnd);

    OsVmemCreateInfo vmemGuestCreateInfo {
        .BaseAddress = OsAnyPointer(memoryBegin),
        .Size = memoryEnd - memoryBegin,
        .Access = eOsMemoryRead | eOsMemoryWrite | eOsMemoryExecute | eOsMemoryCommit | eOsMemoryDiscard,
        .Process = process,
    };
    void *guestAddress = nullptr;
    if (OsStatus status = OsVmemCreate(vmemGuestCreateInfo, &guestAddress)) {
        OsDebugMessage(eOsLogInfo, "Failed to create guest memory");
        return status;
    }

    if (guestAddress != (void*)memoryBegin) {
        OsDebugMessage(eOsLogInfo, "Guest memory allocated at wrong address");
        return OsStatusInvalidData;
    }

    OsVmemMapInfo vmemGuestMapInfo {
        .SrcAddress = OsAddress(guestAddress),
        .Size = memoryEnd - memoryBegin,
        .Access = eOsMemoryWrite,
        .Source = process,
    };
    if (OsStatus status = OsVmemMap(vmemGuestMapInfo, &guestAddress)) {
        OsDebugMessage(eOsLogInfo, "Failed to map guest memory");
        return status;
    }

    for (const elf::ProgramHeader &ph : phs) {
        if (ph.type != elf::ProgramHeaderType::eLoad) {
            continue;
        }

        char *base = reinterpret_cast<char*>(guestAddress) + (ph.vaddr - memoryBegin);
        OsDeviceReadRequest read {
            .BufferFront = base,
            .BufferBack = base + ph.filesz,
            .Offset = ph.offset,
            .Timeout = OS_TIMEOUT_INFINITE,
        };

        RtldPrintf("Loading segment: %p-%p to %p", (void*)ph.vaddr, (void*)(ph.vaddr + ph.memsz), base);

        OsSize size = 0;
        if (OsStatus status = OsDeviceRead(file, read, &size)) {
            OsDebugMessage(eOsLogInfo, "Failed to read program header");
            return status;
        }

        if (size != ph.filesz) {
            OsDebugMessage(eOsLogInfo, "Failed to read full program header");
            return OsStatusInvalidData;
        }

        RtldPrintf("First 16 bytes: %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x",
            (uint8_t)base[0], (uint8_t)base[1], (uint8_t)base[2], (uint8_t)base[3],
            (uint8_t)base[4], (uint8_t)base[5], (uint8_t)base[6], (uint8_t)base[7],
            (uint8_t)base[8], (uint8_t)base[9], (uint8_t)base[10], (uint8_t)base[11],
            (uint8_t)base[12], (uint8_t)base[13], (uint8_t)base[14], (uint8_t)base[15]);


#if 0
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
                OsDebugMessage(eOsLogInfo, "Failed to create guest memory for program header");
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

        // TODO: handle misaligned mappings
        if (OsStatus status = OsVmemMap(vmemGuestMapInfo, &guestAddress)) {
            OsDebugMessage(eOsLogInfo, "Failed to map program header into guest memory");
            return status;
        }
#endif


    }

    if (const elf::ProgramHeader *tls = FindTlsSection(phs)) {
        if (OsStatus status = RtldTlsInit(file, process, *tls, tlsInfo)) {
            OsDebugMessage(eOsLogInfo, "Failed to initialize TLS");
            return status;
        }
    }

    return OsStatusSuccess;
}

OsStatus RtldStartProgram(const RtldStartInfo *StartInfo, OsThreadHandle *OutThread) {
    RtldPrintf("Starting program in process %llx", StartInfo->Process);
    uintptr_t entry = 0;
    RtldTlsInitInfo tlsInfo{};
    if (OsStatus status = RtldMapProgram(StartInfo->Program, StartInfo->Process, &entry, &tlsInfo)) {
        OsDebugMessage(eOsLogInfo, "Failed to map program");
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
        OsDebugMessage(eOsLogInfo, "Failed to create start info memory");
        return status;
    }

    OsVmemMapInfo startInfoMapInfo {
        .SrcAddress = reinterpret_cast<OsAddress>(startInfoGuestAddress),
        .Size = startInfoSize,
        .Access = eOsMemoryRead | eOsMemoryWrite,
        .Source = StartInfo->Process,
    };

    if (OsStatus status = OsVmemMap(startInfoMapInfo, &startInfoGuestAddress)) {
        OsDebugMessage(eOsLogInfo, "Failed to map start info memory");
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
        OsDebugMessage(eOsLogInfo, "Failed to create stack memory");
        return status;
    }

    *startInfo = OsClientStartInfo {
        .TlsInit = tlsInfo.InitAddress,
        .TlsDataSize = tlsInfo.TlsDataSize,
        .TlsBssSize = tlsInfo.TlsBssSize,
    };

    RtldPrintf("Entry point: %p", (void*)entry);
    RtldTls tls{};
    if (tlsInfo.TlsDataSize != 0 || tlsInfo.TlsBssSize != 0) {
        if (OsStatus status = RtldCreateTls(StartInfo->Process, &tlsInfo, &tls)) {
            OsDebugMessage(eOsLogInfo, "Failed to create TLS");
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
        OsDebugMessage(eOsLogInfo, "Failed to create thread");
        return status;
    }

    RtldPrintf("Created thread %llx in process %llx", threadHandle, StartInfo->Process);
    *OutThread = threadHandle;
    return OsStatusSuccess;
}

OsStatus RtldCreateTls(OsProcessHandle Process, struct RtldTlsInitInfo *TlsInfo, RtldTls *OutTlsInfo) {
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
        OsDebugMessage(eOsLogInfo, "Failed to create TLS memory");
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
        OsDebugMessage(eOsLogInfo, "Failed to map TLS memory");
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