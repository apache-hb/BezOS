#include "elf.hpp"

#include "gdt.hpp"
#include "log.hpp"
#include "memory/range.hpp"

#include "util/format.hpp"

OsStatus km::LoadElf(std::unique_ptr<vfs2::IVfsNodeHandle> file, SystemMemory &memory, SystemObjects &objects, ProcessLaunch *result) {
    vfs2::VfsNodeStat stat{};
    if (OsStatus status = file->stat(&stat)) {
        return status;
    }

    elf::ElfHeader header{};
    if (OsStatus status = vfs2::ReadObject(file.get(), &header, 0)) {
        return status;
    }

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

    if (header.endian() != std::endian::little || header.elfClass() != elf::ElfClass::eClass64 || header.objVersion() != 1) {
        KmDebugMessage("[ELF] Unsupported ELF format. Only little endian, 64 bit, version 1 elf programs are supported.\n");
        return OsStatusInvalidVersion;
    }

    if (header.type != elf::ElfType::eExecutable) {
        KmDebugMessage("[ELF] Invalid ELF type. Only EXEC elf programs can be launched.\n");
        return OsStatusInvalidData;
    }

    uint64_t phbegin = header.phoff;
    uint64_t phend = phbegin + header.phnum * header.phentsize;

    if (phend > stat.size) {
        KmDebugMessage("[ELF] Invalid program header range\n");
        return OsStatusInvalidData;
    }

    if (header.phentsize != sizeof(elf::ElfProgramHeader)) {
        KmDebugMessage("[ELF] Invalid program header size\n");
        return OsStatusInvalidData;
    }

    std::unique_ptr<elf::ElfProgramHeader[]> phs{new elf::ElfProgramHeader[header.phnum]};

    if (OsStatus status = vfs2::ReadArray(file.get(), phs.get(), header.phnum, phbegin)) {
        return status;
    }

    sm::RcuSharedPtr<Process> process = objects.createProcess("INIT", x64::Privilege::eUser);

    uint64_t entry = header.entry;

    km::IsrContext regs {
        .rip = entry,
        .cs = (SystemGdt::eLongModeUserCode * 0x8) | 0b11,
        .rflags = 0x202,
        .ss = (SystemGdt::eLongModeUserData * 0x8) | 0b11,
    };

    KmDebugMessage("[ELF] Entry point: ", km::Hex(entry), "\n");

    for (const elf::ElfProgramHeader &ph : std::span(phs.get(), header.phnum)) {
        if (ph.type != 1)
            continue;

        KmDebugMessage("[ELF] Program Header type: ",
            km::Hex(ph.type), ", flags: ", ph.flags, ", offset: ", ph.offset, ", filesize: ", ph.filesz,
            ", memorysize: ", km::Hex(ph.memsz), ", virtual address: ", km::Hex(ph.vaddr),
            ", physical address: ", km::Hex(ph.paddr), ", minimum alignment: ", km::Hex(ph.align), "\n");

        //
        // Allocate memory for the section. Ideally this would use
        // malloc with a hint so that the memory is allocated in lower
        // memory regions.
        //
        size_t offset = (ph.vaddr % x64::kPageSize);
        size_t pages = Pages(ph.memsz + offset);
        void *baseVaddr = (void*)(ph.vaddr - offset);
        void *vaddr = (void*)ph.vaddr;
        PhysicalAddress paddr = memory.pmm.alloc4k(pages);

        km::AddressMapping mapping {
            .vaddr = baseVaddr,
            .paddr = paddr,
            .size = pages * x64::kPageSize,
        };

        memory.vmm.markUsed(mapping.virtualRange());

        //
        // Allocate all the memory as writable during the load phase.
        //
        memory.pt.map(mapping, PageFlags::eWrite);

        vfs2::ReadRequest request {
            .begin = (std::byte*)vaddr,
            .end = (std::byte*)vaddr + ph.filesz,
            .offset = ph.offset,
        };

        KmDebugMessage("[ELF] Section: ", mapping, "\n");

        vfs2::ReadResult readResult{};
        if (OsStatus status = file->read(request, &readResult)) {
            return status;
        }

        if (readResult.read != ph.filesz) {
            KmDebugMessage("[ELF] Failed to read section: ", readResult.read, " != ", ph.filesz, "\n");
            KmDebugMessage("[ELF] Section: ", mapping, "\n");
            return OsStatusInvalidData;
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

        memory.pt.map(mapping, flags);

        sm::RcuSharedPtr<AddressSpace> addressSpace = objects.createAddressSpace("ELF Section", mapping, flags, km::MemoryType::eWriteBack, process);
    }

    if (regs.rip == 0) {
        KmDebugMessage("[ELF] Entry point not found\n");
        return OsStatusInvalidData;
    }

    //
    // Now allocate the stack for the main thread.
    //
    static constexpr size_t kStackSize = 0x4000;
    void *stack = memory.allocate(kStackSize, 0x1000, PageFlags::eUser | PageFlags::eData);
    sm::RcuSharedPtr<Thread> main = objects.createThread("Main", process);

    regs.rbp = (uintptr_t)stack + kStackSize;
    regs.rsp = (uintptr_t)stack + kStackSize;

    main->state = regs;

    process->threads.add(main);

    ProcessLaunch launch = {
        .process = process,
        .main = main,
    };

    *result = launch;
    return OsStatusSuccess;
}
