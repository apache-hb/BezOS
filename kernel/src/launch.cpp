#include "elf.hpp"

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
        KmDebugMessage("[ELF] Invalid ELF header\n");
        return OsStatusInvalidData;
    }

    if (header.endian() != std::endian::little || header.elfClass() != elf::ElfClass::eClass64 || header.objVersion() != 1) {
        KmDebugMessage("[ELF] Unsupported ELF format\n");
        return OsStatusInvalidVersion;
    }

    uint64_t phbegin = header.phoff;
    uint64_t phend = phbegin + header.phnum * header.phentsize;

    if (phend > stat.size) {
        KmDebugMessage("[ELF] Invalid program header range\n");
        return OsStatusInvalidData;
    }

    std::unique_ptr<elf::ElfProgramHeader[]> phs{new elf::ElfProgramHeader[header.phnum]};

    if (OsStatus status = vfs2::ReadArray(file.get(), phs.get(), header.phnum, phbegin)) {
        return status;
    }

    Process *process = objects.createProcess("main", km::Privilege::eUser);

    x64::RegisterState regs {
        .rflags = 0x202,
    };

    uint64_t entry = header.entry;

    KmDebugMessage("[ELF] Entry point: ", km::Hex(entry), "\n");

    for (const elf::ElfProgramHeader &ph : std::span(phs.get(), header.phnum)) {
        if (ph.type != 1)
            continue;

        KmDebugMessage("[ELF] Program Header type: ",
            km::Hex(ph.type), ", flags: ", ph.flags, ", offset: ", ph.offset, ", filesize: ", ph.filesz,
            ", memorysize: ", km::Hex(ph.memsz), ", virtual address: ", km::Hex(ph.vaddr),
            ", physical address: ", km::Hex(ph.paddr), ", minimum alignment: ", km::Hex(ph.align), "\n");

        //
        // This assumes that the program header isn't malicious and remaps
        // itself over the kernel. TODO: use malloc with a hint.
        //
        void *vaddr = (void*)sm::roundup(ph.vaddr, x64::kPageSize);
        PhysicalAddress paddr = memory.pmm.alloc4k(Pages(ph.memsz));

        km::AddressMapping mapping {
            .vaddr = vaddr,
            .paddr = paddr,
            .size = Pages(ph.memsz) * x64::kPageSize,
        };

        //
        // Allocate all the memory as writable during the load phase.
        //
        memory.pt.map(mapping, PageFlags::eWrite);
        vfs2::ReadRequest request {
            .begin = vaddr,
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
            KmDebugMessage(km::HexDump(std::span((const uint8_t*)vaddr, ph.filesz)), "\n");
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

        //
        // TODO: im not sure if this is the correct way to find the entrypoint.
        //
        bool containsEntry = entry >= ph.vaddr && entry < ph.vaddr + ph.memsz;

        if (containsEntry) {
            regs.rip = ((uintptr_t)vaddr + (entry - ph.vaddr));
        }

        AddressSpace *addressSpace = objects.createAddressSpace("main", mapping);

        process->memory.add(addressSpace->id);
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

    // TODO: this is really dumb, figure out stack alignment when im less tired.
    regs.rbp = (uintptr_t)stack + 0x3000;
    regs.rsp = (uintptr_t)stack + 0x3000;

    Thread *main = objects.createThread("main");
    main->state = regs;

    process->threads.add(main->id);

    ProcessLaunch launch = {
        .process = process->id,
        .main = main->id,
    };

    *result = launch;
    return OsStatusSuccess;
}
