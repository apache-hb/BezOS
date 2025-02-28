#include "elf.hpp"

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
