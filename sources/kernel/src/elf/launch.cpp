#include "elf.hpp"

#include "fs2/utils.hpp"
#include "gdt.hpp"
#include "kernel.hpp"
#include "log.hpp"

#include "process/system.hpp"

#include "memory/range.hpp"

#include <bezos/start.h>

#include "process/thread.hpp"
#include "system/system.hpp"
#include "util/format.hpp"
#include "util/defer.hpp"

#if 0
static const elf::ProgramHeader *FindProgramHeader(std::span<const elf::ProgramHeader> phs, elf::ProgramHeaderType type) {
    for (const auto& header : phs) {
        if (header.type == type) {
            return &header;
        }
    }

    return nullptr;
}

static const elf::ProgramHeader *FindDynamicSection(std::span<const elf::ProgramHeader> phs) {
    return FindProgramHeader(phs, elf::ProgramHeaderType::eDynamic);
}

static const elf::ProgramHeader *FindTlsSection(std::span<const elf::ProgramHeader> phs) {
    return FindProgramHeader(phs, elf::ProgramHeaderType::eTls);
}

enum {
    R_AMD64_NONE = 0,
    R_AMD64_64 = 1,
    R_AMD64_PC32 = 2,
    R_AMD64_GOT32 = 3,
    R_AMD64_PLT32 = 4,
    R_AMD64_COPY = 5,
    R_AMD64_GLOB_DAT = 6,
    R_AMD64_JUMP_SLOT = 7,
    R_AMD64_RELATIVE = 8,
    R_AMD64_GOTPCREL = 9,
    R_AMD64_32 = 10,
    R_AMD64_32S = 11,
    R_AMD64_16 = 12,
    R_AMD64_PC16 = 13,
    R_AMD64_8 = 14,
    R_AMD64_PC8 = 15,
};

static OsStatus ApplyRelocations(vfs2::IHandle *file, std::span<const elf::Elf64Dyn> relocs, km::AddressMapping mapping, uintptr_t windowOffset) {
    uint64_t relaSize = 0;
    uint64_t relaCount = 0;
    uint64_t relaStart = 0;
    uint64_t relaEnt = 0;
    for (const elf::Elf64Dyn &dyn : relocs) {
        if (dyn.tag == 0x000000006ffffff9) {
            relaCount = dyn.value;
        } else if (dyn.tag == 0x0000000000000007) {
            relaStart = dyn.value;
        } else if (dyn.tag == 0x0000000000000008) {
            relaSize = dyn.value;
        } else if (dyn.tag == 0x0000000000000009) {
            relaEnt = dyn.value;
        }
    }

    if (relaCount == 0 && relaEnt != 0) {
        relaCount = relaSize / relaEnt;
    }

    if (relaCount == 0 && relaStart == 0 && relaSize == 0 && relaEnt == 0) {
        return OsStatusSuccess;
    }

    if (relaCount == 0 || relaStart == 0 || relaSize == 0 || relaEnt == 0) {
        KmDebugMessage("[ELF] Invalid dynamic section relocations\n");
        return OsStatusInvalidData;
    }

    if (relaSize % relaEnt != 0) {
        KmDebugMessage("[ELF] Invalid dynamic section relocation size\n");
        return OsStatusInvalidData;
    }

    if (relaEnt != sizeof(elf::Elf64Rela)) {
        KmDebugMessage("[ELF] Invalid dynamic section relocation entry size: ", relaEnt, " != ", sizeof(elf::Elf64Rela), "\n");
        return OsStatusInvalidData;
    }

    size_t count = relaSize / relaEnt;
    std::unique_ptr<elf::Elf64Rela[]> relas{new elf::Elf64Rela[count]};
    if (OsStatus status = vfs2::ReadArray(file, relas.get(), count, relaStart)) {
        return status;
    }

    KmDebugMessage("[ELF] Applying ", count, " relocations\n");

    for (size_t i = 0; i < count; i++) {
        const elf::Elf64Rela &rela = relas[i];
        uint64_t *target = (uint64_t*)((uintptr_t)mapping.vaddr + windowOffset + rela.offset);

        switch (rela.info & 0xFFFF) {
        case R_AMD64_NONE:
            break;
        case R_AMD64_RELATIVE:
            *target = (uintptr_t)mapping.vaddr + rela.addend;
            break;
        default:
            KmDebugMessage("[ELF] Unsupported relocation type ", km::Hex(rela.info & 0xFFFF), "\n");
            return OsStatusInvalidData;
        }
    }

    return OsStatusSuccess;
}

static OsStatus ReadTlsInit(vfs2::IHandle *file, const elf::ProgramHeader *tls, km::SystemMemory& memory, km::AddressSpace& ptes, km::TlsInit *mapping) {
    km::AddressMapping tlsMapping{};
    OsStatus status = AllocateMemory(memory.pmmAllocator(), ptes, km::Pages(tls->memsz), &tlsMapping);
    if (status != OsStatusSuccess) {
        KmDebugMessage("[ELF] Failed to allocate ", sm::bytes(tls->memsz), " for ELF program load sections. ", status, "\n");
        return status;
    }

    void *tlsWindow = memory.map(tlsMapping.physicalRange(), km::PageFlags::eData);
    if (tlsWindow == nullptr) {
        KmDebugMessage("[ELF] Failed to map TLS memory\n");
        return OsStatusOutOfMemory;
    }

    vfs2::ReadRequest request {
        .begin = (std::byte*)tlsWindow,
        .end = (std::byte*)tlsWindow + tls->filesz,
        .offset = tls->offset,
    };

    vfs2::ReadResult result{};
    if (OsStatus status = file->read(request, &result)) {
        return status;
    }

    if (result.read != tls->filesz) {
        KmDebugMessage("[ELF] Failed to read TLS section\n");
        return OsStatusInvalidData;
    }

    *mapping = km::TlsInit {
        .mapping = tlsMapping,
        .window = tlsWindow,
        .fileSize = tls->filesz,
        .memSize = tls->memsz,
    };
    return OsStatusSuccess;
}

static char toupper(char c) {
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 'A';
    }

    return c;
}

static OsStatus CreateThread(km::Process *process, km::SystemMemory& memory, km::SystemObjects& objects, uintptr_t entry, stdx::String name, km::Thread **result) {
    km::IsrContext regs {
        .rip = entry,
        .cs = (km::SystemGdt::eLongModeUserCode * 0x8) | 0b11,
        .rflags = 0x202,
        .ss = (km::SystemGdt::eLongModeUserData * 0x8) | 0b11,
    };

    KmDebugMessage("[ELF] Entry point: ", km::Hex(regs.rip), "\n");

    //
    // Now allocate the stack for the main thread.
    //

    static constexpr size_t kStackSize = 0x4000;
    km::PageFlags flags = km::PageFlags::eUser | km::PageFlags::eData;
    km::AddressMapping mapping{};
    if (OsStatus status = AllocateMemory(memory.pmmAllocator(), *process->ptes.get(), 4, &mapping)) {
        KmDebugMessage("[ELF] Failed to allocate stack memory: ", status, "\n");
        return status;
    }

    KmDebugMessage("[ELF] Stack: ", mapping, "\n");

    char *stack = (char*)memory.map(mapping.physicalRange(), flags);
    memset(stack, 0x00, kStackSize);
    memory.unmap(stack, kStackSize);

    km::Thread *main = objects.createThread(stdx::String(name), process);
    main->userStack = mapping;

    regs.rbp = (uintptr_t)mapping.vaddr + kStackSize;
    regs.rsp = (uintptr_t)mapping.vaddr + kStackSize;

    main->state = regs;
    *result = main;
    return OsStatusSuccess;
}
#endif

template<typename T>
static OsStatus DeviceReadObject(sys2::InvokeContext *invoke, OsDeviceHandle file, OsSize offset, T *result) {
    OsDeviceReadRequest request {
        .BufferFront = (void*)result,
        .BufferBack = (void*)((char*)result + sizeof(T)),
        .Offset = offset,
    };
    OsSize read = 0;
    if (OsStatus status = sys2::SysDeviceRead(invoke, file, request, &read)) {
        return status;
    }

    if (read != sizeof(T)) {
        return OsStatusInvalidData;
    }

    return OsStatusSuccess;
}

template<typename T>
static OsStatus DeviceReadArray(sys2::InvokeContext *invoke, OsDeviceHandle file, OsSize offset, std::span<T> result) {
    OsDeviceReadRequest request {
        .BufferFront = (void*)result.data(),
        .BufferBack = (void*)((char*)result.data() + result.size_bytes()),
        .Offset = offset,
    };
    OsSize read = 0;
    if (OsStatus status = sys2::SysDeviceRead(invoke, file, request, &read)) {
        return status;
    }

    if (read != result.size_bytes()) {
        return OsStatusInvalidData;
    }

    return OsStatusSuccess;
}

static OsStatus MapProgram(sys2::InvokeContext *invoke, OsDeviceHandle file, OsProcessHandle process, uintptr_t *entry) {
    elf::Header header{};
    OsFileInfo info{};

    if (OsStatus status = sys2::SysDeviceInvoke(invoke, file, eOsFileStat, &info, sizeof(info))) {
        KmDebugMessage("[ELF] Failed to stat file info. ", status, "\n");
        return status;
    }

    if (OsStatus status = DeviceReadObject(invoke, file, 0, &header)) {
        KmDebugMessage("[ELF] Failed to read elf header. ", status, "\n");
        return status;
    }

    if (OsStatus status = km::detail::ValidateElfHeader(header, info.LogicalSize)) {
        KmDebugMessage("[ELF] Invalid ELF header. ", status, "\n");
        return status;
    }

    size_t phOff = header.phoff;
    size_t phNum = header.phnum;
    size_t phEnt = header.phentsize;
    if (phOff == 0 || phNum == 0 || phEnt == 0) {
        KmDebugMessage("[ELF] Invalid program header\n");
        return OsStatusInvalidData;
    }

    sm::FixedArray<elf::ProgramHeader> phArray{phNum};
    if (OsStatus status = DeviceReadArray(invoke, file, phOff, std::span(phArray))) {
        KmDebugMessage("[ELF] Failed to read program headers. ", status, "\n");
        return status;
    }

    if (header.entry == 0) {
        KmDebugMessage("[ELF] Invalid entry point\n");
        return OsStatusInvalidData;
    }

    for (elf::ProgramHeader ph : phArray) {
        if (ph.type != elf::ProgramHeaderType::eLoad) {
            continue;
        }
        void *guestAddress = nullptr;

        OsMemoryAccess access = eOsMemoryCommit;
        if (ph.flags & (1 << 0))
            access |= eOsMemoryExecute;
        if (ph.flags & (1 << 1))
            access |= eOsMemoryWrite;
        if (ph.flags & (1 << 2))
            access |= eOsMemoryRead;

        if (ph.memsz != ph.filesz) {
            OsVmemCreateInfo vmemGuestCreateInfo {
                .BaseAddress = (void*)ph.vaddr,
                .Size = sm::roundup(ph.memsz, x64::kPageSize),
                .Access = access | eOsMemoryDiscard,
                .Process = process,
            };

            if (OsStatus status = sys2::SysVmemCreate(invoke, vmemGuestCreateInfo, &guestAddress)) {
                KmDebugMessage("[ELF] Failed to create guest memory. ", status, "\n");
                return status;
            }
        }

        OsVmemMapInfo vmemGuestMapInfo {
            .SrcAddress = (uintptr_t)ph.offset,
            .DstAddress = ph.vaddr,
            .Size = ph.filesz,
            .Access = access,
            .Source = file,
            .Process = process,
        };

        if (OsStatus status = sys2::SysVmemMap(invoke, vmemGuestMapInfo, &guestAddress)) {
            KmDebugMessage("[ELF] Failed to map guest memory. ", status, "\n");
            return status;
        }
    }

    *entry = header.entry;
    return OsStatusSuccess;
}

OsStatus km::LoadElf2(sys2::InvokeContext *invoke, OsDeviceHandle file, OsProcessHandle *process, OsThreadHandle *thread) {
    OsStatus status = OsStatusSuccess;
    OsProcessHandle hProcess = OS_HANDLE_INVALID;
    OsThreadHandle hThread = OS_HANDLE_INVALID;
    void *startInfoGuestAddress = nullptr;
    void *startInfoAddress = nullptr;
    void *stackGuestAddress = nullptr;
    OsClientStartInfo *startInfo = nullptr;
    uintptr_t entry = 0;
    uintptr_t base = 0;

    OsProcessCreateInfo processCreateInfo;
    OsThreadCreateInfo threadCreateInfo;
    OsVmemCreateInfo vmemCreateInfo;
    OsVmemMapInfo vmemMapInfo;
    OsVmemCreateInfo stackCreateInfo;

    processCreateInfo = OsProcessCreateInfo {
        .Name = "INIT.ELF",
        .Flags = eOsProcessSuspended,
    };

    if ((status = sys2::SysProcessCreate(invoke, processCreateInfo, &hProcess))) {
        return status;
    }

    if ((status = MapProgram(invoke, file, hProcess, &entry))) {
        goto cleanup;
    }

    vmemCreateInfo = OsVmemCreateInfo {
        .Size = 0x1000,
        .Access = eOsMemoryRead,
        .Process = hProcess,
    };

    if ((status = sys2::SysVmemCreate(invoke, vmemCreateInfo, &startInfoGuestAddress))) {
        KmDebugMessage("[ELF] Failed to create start info memory. ", status, "\n");
        goto cleanup;
    }

    vmemMapInfo = OsVmemMapInfo {
        .SrcAddress = (uintptr_t)startInfoGuestAddress,
        .Size = 0x1000,
        .Access = eOsMemoryRead | eOsMemoryWrite,
        .Source = hProcess,
    };

    if ((status = sys2::SysVmemMap(invoke, vmemMapInfo, &startInfoAddress))) {
        KmDebugMessage("[ELF] Failed to map start info memory into launcher. ", status, "\n");
        goto cleanup;
    }

    stackCreateInfo = OsVmemCreateInfo {
        .Size = 0x4000,
        .Access = eOsMemoryRead | eOsMemoryWrite | eOsMemoryDiscard,
        .Process = hProcess,
    };

    if ((status = sys2::SysVmemCreate(invoke, stackCreateInfo, &stackGuestAddress))) {
        KmDebugMessage("[ELF] Failed to create stack memory. ", status, "\n");
        goto cleanup;
    }

    base = (uintptr_t)stackGuestAddress + 0x4000;

    startInfo = (OsClientStartInfo*)startInfoAddress;

    *startInfo = OsClientStartInfo {
        .TlsInit = nullptr,
        .TlsInitSize = 0,
    };

    threadCreateInfo = OsThreadCreateInfo {
        .Name = "INIT MASTER THREAD",
        .CpuState = OsMachineContext {
            .rdi = (uintptr_t)startInfoGuestAddress,
            .rbp = base,
            .rsp = base,
            .rip = entry,
        },
        .Flags = eOsThreadSuspended,
        .Process = hProcess,
    };

    if ((status = sys2::SysThreadCreate(invoke, threadCreateInfo, &hThread))) {
        KmDebugMessage("[ELF] Failed to launch init process thread. ", status, "\n");
        goto cleanup;
    }

    *process = hProcess;
    *thread = hThread;

    sys2::SysVmemRelease(invoke, startInfoAddress, 0x1000);
    return OsStatusSuccess;

cleanup:
    if (hThread != OS_HANDLE_INVALID) {
        sys2::SysThreadDestroy(invoke, eOsThreadFinished, hThread);
    }

    if (startInfoAddress != nullptr) {
        sys2::SysVmemRelease(invoke, startInfoAddress, 0x1000);
    }

    if (hProcess != OS_HANDLE_INVALID) {
        sys2::SysProcessDestroy(invoke, hProcess, 0, eOsProcessExited);
    }

    return status;
}

#if 0
OsStatus km::LoadElf(std::unique_ptr<vfs2::IFileHandle> file, SystemMemory& memory, SystemObjects& objects, ProcessLaunch *result) {
    auto node = vfs2::GetHandleNode(file.get());
    vfs2::NodeInfo nInfo = node->info();

    MemoryRange pteMemory = memory.pmmAllocate(256);

    stdx::String name = stdx::String(nInfo.name);
    for (char& c : name) {
        c = toupper(c);
    }

    ProcessCreateInfo createInfo {
        .parent = nullptr,
        .privilege = x64::Privilege::eUser,
    };

    Process *process = nullptr;
    if (OsStatus status = objects.createProcess(stdx::String(name), pteMemory, createInfo, &process)) {
        return status;
    }

    Program program{};
    if (OsStatus status = LoadElfProgram(file.get(), memory, process, &program)) {
        return status;
    }

    process->tlsInit = program.tlsInit;

    Thread *main = nullptr;
    if (OsStatus status = CreateThread(process, memory, objects, program.entry, name + " MAIN", &main)) {
        return status;
    }

    ProcessLaunch launch = {
        .process = process,
        .main = main,
    };

    KmDebugMessage("[ELF] Launching process: ", km::Hex(process->publicId()), "\n");

    *result = launch;
    return OsStatusSuccess;
}

OsStatus km::LoadElfProgram(vfs2::IFileHandle *file, SystemMemory& memory, Process *process, Program *result) {
    OsFileInfo stat{};
    if (OsStatus status = file->stat(&stat)) {
        return status;
    }

    elf::Header header{};
    if (OsStatus status = vfs2::ReadObject(file, &header, 0)) {
        return status;
    }

    if (OsStatus status = detail::ValidateElfHeader(header, stat.LogicalSize)) {
        return status;
    }

    sm::FixedArray<elf::ProgramHeader> phs{header.phnum};

    if (OsStatus status = vfs2::ReadArray(file, phs.data(), header.phnum, header.phoff)) {
        return status;
    }

    km::VirtualRange loadMemory{};
    if (OsStatus status = detail::LoadMemorySize(phs, &loadMemory)) {
        KmDebugMessage("[ELF] Failed to calculate load memory size. ", status, "\n");
        return status;
    }

    KmDebugMessage("[ELF] Load memory range: ", loadMemory, "\n");

    km::AddressMapping loadMapping{};
    OsStatus status = AllocateMemory(memory.pmmAllocator(), *process->ptes.get(), Pages(loadMemory.size()), loadMemory.front, &loadMapping);
    if (status != OsStatusSuccess) {
        KmDebugMessage("[ELF] Failed to allocate ", sm::bytes(loadMemory.size()), " for ELF program load sections. ", status, "\n");
        return status;
    }

    KmDebugMessage("[ELF] Load memory mapping: ", loadMapping, "\n");

    char *userWindow = (char*)memory.map(loadMapping.physicalRange(), PageFlags::eData);
    if (userWindow == nullptr) {
        KmDebugMessage("[ELF] Failed to map user window\n");
        return OsStatusOutOfMemory;
    }

    defer {
        memory.unmap(userWindow, loadMemory.size());
    };

    uintptr_t windowOffset = (uintptr_t)userWindow - (uintptr_t)loadMapping.vaddr;

    const elf::ProgramHeader *tls = FindTlsSection(phs);
    if (tls != nullptr) {
        KmDebugMessage("[ELF] TLS section found\n");
    }

    const elf::ProgramHeader *dynamic = FindDynamicSection(phs);
    std::unique_ptr<elf::Elf64Dyn[]> dyn;
    if (dynamic == nullptr) {
        KmDebugMessage("[ELF] Dynamic section not found\n");
    } else {
        if (dynamic->filesz % sizeof(elf::Elf64Dyn) != 0) {
            KmDebugMessage("[ELF] Invalid dynamic section size\n");
            return OsStatusInvalidData;
        }

        size_t count = dynamic->filesz / sizeof(elf::Elf64Dyn);
        dyn.reset(new elf::Elf64Dyn[count]);
        if (OsStatus status = vfs2::ReadArray(file, dyn.get(), count, dynamic->offset)) {
            return status;
        }
    }

    for (const elf::ProgramHeader &ph : phs) {
        if (ph.type != elf::ProgramHeaderType::eLoad)
            continue;

        KmDebugMessage("[ELF] Program Header type: ",
            km::Hex(std::to_underlying(ph.type)), ", flags: ", km::Hex(ph.flags), ", offset: ", km::Hex(ph.offset), ", filesize: ", km::Hex(ph.filesz),
            ", memorysize: ", km::Hex(ph.memsz), ", virtual address: ", km::Hex(ph.vaddr), ", align: ", km::Hex(ph.align), "\n");

        //
        // Distance between this program header load base and the ELF load base.
        //
        uintptr_t alignedVaddr = ph.vaddr;
        uintptr_t distance = (uintptr_t)alignedVaddr - (uintptr_t)loadMemory.front;

        //
        // The virtual address of the program header load address.
        //
        uintptr_t base = (uintptr_t)loadMapping.vaddr + distance;
        size_t offset = (base % x64::kPageSize);
        void *vaddr = (void*)(base);

        void *baseVaddr = (void*)(base - offset);
        size_t pages = Pages(ph.memsz + offset);
        km::AddressMapping mapping {
            .vaddr = baseVaddr,
            .paddr = loadMapping.paddr + distance,
            .size = pages * x64::kPageSize,
        };

        vfs2::ReadRequest request {
            .begin = (std::byte*)vaddr + windowOffset,
            .end = (std::byte*)vaddr + windowOffset + ph.filesz,
            .offset = ph.offset,
        };

        KmDebugMessage("[ELF] Section: ", km::Hex(base), " ", vaddr, " ", mapping, "\n");

        vfs2::ReadResult readResult{};
        if (OsStatus status = file->read(request, &readResult)) {
            return status;
        }

        if (readResult.read != ph.filesz) {
            KmDebugMessage("[ELF] Failed to read section: ", readResult.read, " != ", ph.filesz, "\n");
            KmDebugMessage("[ELF] Section: ", mapping, "\n");
            return OsStatusInvalidData;
        }

        if (ph.memsz > ph.filesz) {
            uint64_t bssSize = ph.memsz - ph.filesz;
            memset((char*)vaddr + windowOffset + ph.filesz, 0x00, bssSize);
        }

#if 0
        //
        // Now we can set the correct flags.
        //
        PageFlags flags = PageFlags::eUser;
        if (ph.flags & (1 << 0))
            flags |= PageFlags::eExecute;
        if (ph.flags & (1 << 1))
            flags |= PageFlags::eWrite;
        if (ph.flags & (1 << 2))
            flags |= PageFlags::eRead;

        process->ptes.map(mapping, flags);
#endif
    }

    if (dyn) {
        if (OsStatus status = ApplyRelocations(file, std::span(dyn.get(), dynamic->filesz / sizeof(elf::Elf64Dyn)), loadMapping, windowOffset)) {
            return status;
        }
    }

    //
    // Now allocate the stack for the main thread.
    //
    km::TlsInit tlsInit{};

    if (tls) {
        if (OsStatus status = ReadTlsInit(file, tls, memory, *process->ptes.get(), &tlsInit)) {
            return status;
        }
    }

    uintptr_t entry = ((uintptr_t)header.entry - (uintptr_t)loadMemory.front) + (uintptr_t)loadMapping.vaddr;

    *result = Program {
        .tlsInit = tlsInit,
        .loadMapping = loadMapping,
        .entry = entry,
    };

    return OsStatusSuccess;
}
#endif
