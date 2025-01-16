#include "elf.hpp"

#include "log.hpp"

std::expected<km::Process, bool> km::LoadElf(std::span<const uint8_t> program) {
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

    for (const elf::ElfProgramHeader &ph : phs) {
        KmDebugMessage("[ELF] Program Header type: ",
            km::Hex(ph.type), ", flags: ", ph.flags, ", offset:", ph.offset, ", filesize: ", ph.filesz,
            ", memorysize: ", ph.memsz, ", virtual address: ", ph.vaddr,
            ", physical address: ", ph.paddr, ", minimum alignment: ", ph.align, "\n");
    }

    return std::unexpected{false};
}
