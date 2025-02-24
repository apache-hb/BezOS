#pragma once

#include "boot.hpp"
#include "isr/isr.hpp"
#include "memory/paging.hpp"
#include "pat.hpp"

namespace km {
    PageMemoryTypeLayout GetDefaultPatLayout();
    PageMemoryTypeLayout SetupPat();

    void SetupMtrrs(x64::MemoryTypeRanges& mtrrs, const km::PageBuilder& pm);
    void WriteMtrrs(const km::PageBuilder& pm);

    void WriteMemoryMap(const boot::MemoryMap& memmap);

    void DumpIsrState(const km::IsrContext *context);
    void DumpStackTrace(const km::IsrContext *context);
    void DumpIsrContext(const km::IsrContext *context, stdx::StringView message);
    void InstallExceptionHandlers(SharedIsrTable *ist);

    void InitGlobalAllocator(void *memory, size_t size);
}
