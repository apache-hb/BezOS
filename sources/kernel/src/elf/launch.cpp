#include "elf.hpp"

#include "gdt.hpp"
#include "log.hpp"
#include "memory/range.hpp"

#include "util/format.hpp"

static OsStatus ValidateElfHeader(const elf::Header &header, size_t size) {
    if (!header.isValid()) {
        KmDebugMessage(
            "[ELF] Invalid ELF header. Invalid header magic {",
            km::Hex(header.ident[0]).pad(2), ", ",
            km::Hex(header.ident[1]).pad(2), ", ",
            km::Hex(header.ident[2]).pad(2), ", ",
            km::Hex(header.ident[3]).pad(2), "}.\n"
        );

        return OsStatusInvalidData;
    }

    if (header.endian() != std::endian::little || header.elfClass() != elf::Class::eClass64 || header.objVersion() != 1) {
        KmDebugMessage("[ELF] Unsupported ELF format. Only little endian, 64 bit, version 1 elf programs are supported.\n");
        return OsStatusInvalidVersion;
    }

    if (header.type != elf::Type::eExecutable && header.type != elf::Type::eShared) {
        KmDebugMessage("[ELF] Invalid ELF type. Only EXEC & DYN elf programs can be launched.\n");
        return OsStatusInvalidData;
    }

    uint64_t phbegin = header.phoff;
    uint64_t phend = phbegin + header.phnum * header.phentsize;

    if (phend > size) {
        KmDebugMessage("[ELF] Invalid program header range\n");
        return OsStatusInvalidData;
    }

    if (header.phentsize != sizeof(elf::ProgramHeader)) {
        KmDebugMessage("[ELF] Invalid program header size\n");
        return OsStatusInvalidData;
    }

    if (header.entry == 0) {
        KmDebugMessage("[ELF] Invalid entry point\n");
        return OsStatusInvalidData;
    }

    return OsStatusSuccess;
}

static const elf::ProgramHeader *FindDynamicSection(std::span<const elf::ProgramHeader> phs) {
    for (const auto& header : phs) {
        if (header.type == elf::ProgramHeaderType::eDynamic) {
            return &header;
        }
    }

    return nullptr;
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

static OsStatus ApplyRelocations(vfs2::IVfsNodeHandle *file, std::span<const elf::Elf64Dyn> relocs, km::AddressMapping mapping) {
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
        uint64_t *target = (uint64_t*)((uintptr_t)mapping.vaddr + rela.offset);
        // uint64_t *symbol = (uint64_t*)((uintptr_t)mapping.vaddr + rela.addend);

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

OsStatus km::LoadElf(std::unique_ptr<vfs2::IVfsNodeHandle> file, SystemMemory &memory, SystemObjects &objects, ProcessLaunch *result) {
    vfs2::VfsNodeStat stat{};
    if (OsStatus status = file->stat(&stat)) {
        return status;
    }

    elf::Header header{};
    if (OsStatus status = vfs2::ReadObject(file.get(), &header, 0)) {
        return status;
    }

    if (OsStatus status = ValidateElfHeader(header, stat.size)) {
        return status;
    }

    std::unique_ptr<elf::ProgramHeader[]> phs{new elf::ProgramHeader[header.phnum]};
    std::span<const elf::ProgramHeader> programHeaders = std::span(phs.get(), header.phnum);

    if (OsStatus status = vfs2::ReadArray(file.get(), phs.get(), header.phnum, header.phoff)) {
        return status;
    }

    km::VirtualRange loadMemory{};
    if (OsStatus status = detail::LoadMemorySize(programHeaders, &loadMemory)) {
        KmDebugMessage("[ELF] Failed to calculate load memory size. ", status, "\n");
        return status;
    }

    MemoryRange pteMemory = memory.pmmAllocate(256);

    sm::RcuSharedPtr<Process> process = objects.createProcess("INIT", x64::Privilege::eUser, &memory.pageTables(), pteMemory);

    // TODO: use a window rather than swapping the active map.
    memory.getPager().setActiveMap(process->ptes.root());

    KmDebugMessage("[ELF] Load memory range: ", loadMemory, "\n");

    km::AddressMapping loadMapping{};
    OsStatus status = AllocateMemory(memory.pmmAllocator(), &process->ptes, Pages(loadMemory.size()), loadMemory.front, &loadMapping);
    if (status != OsStatusSuccess) {
        KmDebugMessage("[ELF] Failed to allocate ", sm::bytes(loadMemory.size()), " for ELF program load sections. ", status, "\n");
        return status;
    }

    KmDebugMessage("[ELF] Load memory mapping: ", loadMapping, "\n");

    km::IsrContext regs {
        .rip = ((uintptr_t)header.entry - (uintptr_t)loadMemory.front) + (uintptr_t)loadMapping.vaddr,
        .cs = (SystemGdt::eLongModeUserCode * 0x8) | 0b11,
        .rflags = 0x202,
        .ss = (SystemGdt::eLongModeUserData * 0x8) | 0b11,
    };

    KmDebugMessage("[ELF] Entry point: ", km::Hex(regs.rip), "\n");

    const elf::ProgramHeader *dynamic = FindDynamicSection(programHeaders);
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
        if (OsStatus status = vfs2::ReadArray(file.get(), dyn.get(), count, dynamic->offset)) {
            return status;
        }
    }

    for (const elf::ProgramHeader &ph : programHeaders) {
        if (ph.type != elf::ProgramHeaderType::eLoad)
            continue;

        KmDebugMessage("[ELF] Program Header type: ",
            km::Hex(std::to_underlying(ph.type)), ", flags: ", km::Hex(ph.flags), ", offset: ", km::Hex(ph.offset), ", filesize: ", km::Hex(ph.filesz),
            ", memorysize: ", km::Hex(ph.memsz), ", virtual address: ", km::Hex(ph.vaddr),
            ", physical address: ", km::Hex(ph.paddr), ", minimum alignment: ", km::Hex(ph.align), "\n");

        //
        // Line up the virtual address against the load memory.
        //

        // Distance between this program header load base and the ELF load base.
        uintptr_t alignedVaddr = ph.vaddr;
        uintptr_t distance = (uintptr_t)alignedVaddr - (uintptr_t)loadMemory.front;

        // The virtual address of the program header load address.
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
            .begin = (std::byte*)vaddr,
            .end = (std::byte*)vaddr + ph.filesz,
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
            memset((char*)vaddr + ph.filesz, 0x00, bssSize);
        }

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

#if 0
        process->ptes.map(mapping, flags);
#endif

        objects.createAddressSpace("ELF SECTION", mapping, flags, km::MemoryType::eWriteBack, process);
    }

    if (dyn != nullptr) {
        if (OsStatus status = ApplyRelocations(file.get(), std::span(dyn.get(), dynamic->filesz / sizeof(elf::Elf64Dyn)), loadMapping)) {
            return status;
        }
    }

    if (regs.rip == 0) {
        KmDebugMessage("[ELF] Entry point not found\n");
        return OsStatusInvalidData;
    }

    //
    // Now allocate the stack for the main thread.
    //

    static constexpr size_t kStackSize = 0x4000;
    PageFlags flags = PageFlags::eUser | PageFlags::eData;
    MemoryType type = MemoryType::eWriteBack;
    AddressMapping mapping{};
    if (OsStatus status = AllocateMemory(memory.pmmAllocator(), &process->ptes, 4, &mapping)) {
        KmDebugMessage("[ELF] Failed to allocate stack memory: ", status, "\n");
        return status;
    }

    memset((void*)mapping.vaddr, 0x00, kStackSize);

    sm::RcuSharedPtr<AddressSpace> stackSpace = objects.createAddressSpace("MAIN STACK", mapping, flags, type, process);

    sm::RcuSharedPtr<Thread> main = objects.createThread("MAIN", process);
    main->stack = stackSpace;

    regs.rbp = (uintptr_t)mapping.vaddr + kStackSize;
    regs.rsp = (uintptr_t)mapping.vaddr + kStackSize;

    main->state = regs;

    ProcessLaunch launch = {
        .process = process,
        .main = main,
    };

    KmDebugMessage("[ELF] Launching process: ", km::Hex(process->id()), "\n");

    *result = launch;
    return OsStatusSuccess;
}
