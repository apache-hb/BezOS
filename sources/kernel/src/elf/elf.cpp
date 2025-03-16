#include "elf.hpp"
#include "kernel.hpp"
#include "log.hpp"
#include "process/process.hpp"
#include "process/thread.hpp"
#include "util/defer.hpp"

OsStatus km::detail::LoadMemorySize(std::span<const elf::ProgramHeader> phs, km::VirtualRange *range) {
    km::VirtualRange result{(void*)UINTPTR_MAX, 0};

    for (const auto& header : phs) {
        if (header.type != elf::ProgramHeaderType::eLoad) continue;

        uint64_t vaddr = sm::roundup(header.vaddr, header.align);
        if (vaddr < (uintptr_t)result.back) {
            vaddr = sm::roundup((uintptr_t)result.back, header.align);
        }

        result.front = (void*)std::min((uintptr_t)result.front, vaddr);
        result.back = (void*)std::max((uintptr_t)result.back, vaddr + header.memsz);
    }

    if (!result.isValid() || result.isEmpty()) {
        return OsStatusInvalidData;
    }

    *range = km::alignedOut(result, x64::kPageSize);
    return OsStatusSuccess;
}

OsStatus km::detail::ValidateElfHeader(const elf::Header &header, size_t size) {
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

    if (header.phentsize != sizeof(elf::ProgramHeader)) {
        KmDebugMessage("[ELF] Invalid program header size.\n");
        return OsStatusInvalidData;
    }

    uint64_t phbegin = header.phoff;
    uint64_t phend = phbegin + header.phnum * header.phentsize;

    if (phend > size) {
        KmDebugMessage("[ELF] Invalid program header range.\n");
        return OsStatusInvalidData;
    }

    if (header.entry == 0) {
        KmDebugMessage("[ELF] Program has no entry point.\n");
        return OsStatusInvalidData;
    }

    return OsStatusSuccess;
}

OsStatus km::CreateTls(Thread *thread, const Program& program) {
    km::AddressMapping tlsMapping{};
    if (OsStatus status = thread->process->map(Pages(program.tlsInit.size() + sizeof(uintptr_t)), PageFlags::eUser, MemoryType::eWriteBack, &tlsMapping)) {
        return status;
    }

    SystemMemory *memory = km::GetSystemMemory();
    char *tlsWindow = (char*)memory->map(tlsMapping.physicalRange(), PageFlags::eData);
    if (tlsWindow == nullptr) {
        return OsStatusOutOfMemory;
    }

    defer {
        memory->unmap(tlsWindow, program.tlsInit.size() + sizeof(uintptr_t));
    };

    memcpy(tlsWindow, program.tlsInit.data(), program.tlsInit.size());

    // Setup the TLS self pointer
    const void *vaddr = (char*)tlsMapping.vaddr + program.tlsInit.size();
    memcpy(tlsWindow + program.tlsInit.size(), &vaddr, sizeof(uintptr_t));

    thread->tlsAddress = (uintptr_t)vaddr;
    thread->tlsMapping = tlsMapping;

    return OsStatusSuccess;
}
