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

static OsStatus InitNewThreadTls(km::TlsInit tlsInit, km::SystemMemory& memory, km::Process *process, km::TlsMapping *mapping) {
    if (!tlsInit.present()) {
        return OsStatusSuccess;
    }

    km::AddressMapping tlsMapping{};
    OsStatus status = process->map(memory, km::Pages(tlsInit.memSize + sizeof(uintptr_t)), km::PageFlags::eUserData, km::MemoryType::eWriteBack, &tlsMapping);
    if (status != OsStatusSuccess) {
        KmDebugMessage("[ELF] Failed to allocate ", sm::bytes(tlsInit.memSize + sizeof(uintptr_t)), " for ELF program load sections. ", status, "\n");
        return status;
    }

    void *tlsWindow = memory.map(tlsMapping.physicalRange(), km::PageFlags::eData);
    if (tlsWindow == nullptr) {
        KmDebugMessage("[ELF] Failed to map TLS memory\n");
        return OsStatusOutOfMemory;
    }

    defer {
        memory.unmap(tlsWindow, tlsMapping.size);
    };

    km::TlsMapping newMapping {
        .mapping = tlsMapping,
        .memSize = tlsInit.memSize,
    };

    memcpy(tlsWindow, tlsInit.window, tlsInit.fileSize);

    size_t bssSize = tlsInit.bssSize();
    if (bssSize > 0) {
        KmDebugMessage("[ELF] TLS BSS size: ", bssSize, " ", tlsMapping, "\n");
        memset(((char*)tlsWindow + tlsInit.fileSize), 0, bssSize);
    }

    const void *vaddr = (char*)tlsMapping.vaddr + tlsInit.memSize;
    memcpy((char*)tlsWindow + tlsInit.memSize, &vaddr, sizeof(uintptr_t));

    *mapping = newMapping;
    return OsStatusSuccess;
}

OsStatus km::TlsInit::createTls(SystemMemory& memory, Thread *thread) {
    if (!present()) {
        return OsStatusSuccess;
    }

    km::TlsMapping tlsMapping{};
    if (OsStatus status = InitNewThreadTls(*this, memory, thread->process, &tlsMapping)) {
        return status;
    }

    thread->tlsAddress = (uintptr_t)tlsMapping.tlsAddress();
    return OsStatusSuccess;
}

OsStatus km::Program::createTls(SystemMemory& memory, Thread *thread) {
    return tlsInit.createTls(memory, thread);
}
