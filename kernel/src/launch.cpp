#include "elf.hpp"

#include "allocator/tlsf.hpp"
#include "log.hpp"

std::expected<km::Process, bool> km::LoadElf(std::span<const uint8_t> program, stdx::StringView name, uint32_t id, SystemMemory& memory, mem::IAllocator *allocator) {
    KmDebugMessage("[ELF] Launching process from ELF\n");

    if (program.size() < sizeof(elf::ElfHeader)) {
        KmDebugMessage("[ELF] Program too small\n");
        return std::unexpected{false};
    }

    const elf::ElfHeader *header = reinterpret_cast<const elf::ElfHeader *>(program.data());

    if (!header->isValid()) {
        KmDebugMessage("[ELF] Invalid ELF header\n");
        return std::unexpected{false};
    }

    if (header->endian() != std::endian::little || header->elfClass() != elf::ElfClass::eClass64 || header->objVersion() != 1) {
        KmDebugMessage("[ELF] Unsupported ELF format\n");
        return std::unexpected{false};
    }

    uint64_t phbegin = header->phoff;
    uint64_t phend = phbegin + header->phnum * header->phentsize;

    if (phend > program.size()) {
        KmDebugMessage("[ELF] Invalid program header range\n");
        return std::unexpected{false};
    }

    std::span<const elf::ElfProgramHeader> phs{reinterpret_cast<const elf::ElfProgramHeader *>(program.data() + phbegin), header->phnum};

    KmDebugMessage("[ELF] Program Header Count: ", phs.size(), "\n");

    stdx::Vector<void*> processMemory(allocator);

    ProcessThread main = {
        .name = "main",
        .threadId = 1,
    };

    void *stack = memory.allocate(0x1000, 0x1000, PageFlags::eUser | PageFlags::eWrite);

    main.state = MachineState {
        .rbp = (uintptr_t)stack + 0x1000,
        .rsp = (uintptr_t)stack + 0x1000,
    };

    uint64_t entry = header->entry;

    KmDebugMessage("[ELF] Entry point: ", km::Hex(entry), "\n");

    for (const elf::ElfProgramHeader &ph : phs) {
        KmDebugMessage("[ELF] Program Header type: ",
            km::Hex(ph.type), ", flags: ", ph.flags, ", offset: ", ph.offset, ", filesize: ", ph.filesz,
            ", memorysize: ", km::Hex(ph.memsz), ", virtual address: ", km::Hex(ph.vaddr),
            ", physical address: ", km::Hex(ph.paddr), ", minimum alignment: ", km::Hex(ph.align), "\n");

        if (ph.type != 1)
            continue;

        // TODO: dont mark everything as writable
        PageFlags flags = PageFlags::eUser | PageFlags::eWrite;
        if (ph.flags & (1 << 0))
            flags |= PageFlags::eExecute;
        if (ph.flags & (1 << 1))
            flags |= PageFlags::eWrite;
        if (ph.flags & (1 << 2))
            flags |= PageFlags::eRead;

        KmDebugMessage("[ELF] Flags: ", std::to_underlying(flags), "\n");

        void *vaddr = memory.allocate(ph.memsz, ph.align, flags);
        KmDebugMessage("[ELF] Allocated memory at ", km::Hex((uintptr_t)vaddr), " for ", km::Hex(ph.memsz), " bytes\n");
        memcpy(vaddr, program.data() + ph.offset, ph.filesz);

        bool containsEntry = entry >= ph.vaddr && entry < ph.vaddr + ph.memsz;

        if (containsEntry) {
            main.state.rip = ((uintptr_t)vaddr + (entry - ph.vaddr));
        }

        mem::TlsfAllocator *alloc = static_cast<mem::TlsfAllocator*>(allocator);
        // KmDebugMessage("Check: ", tlsf_check(alloc->getAllocator()), "\n");
        tlsf_walk_pool(alloc->getAllocator(), [](void *ptr, size_t size, int used, void *) {
            KmDebugMessage("[TLSF] ", ptr, " ", size, " ", used, "\n");
        }, nullptr);

        processMemory.add(vaddr);
    }

    if (main.state.rip == 0) {
        KmDebugMessage("[ELF] Entry point not found\n");
        return std::unexpected{false};
    }

    return Process {
        .name = name,
        .processId = id,
        .memory = std::move(processMemory),
        .main = main,
    };
}
