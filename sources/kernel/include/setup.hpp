#pragma once

#include "boot.hpp"
#include "isr/isr.hpp"
#include "memory/paging.hpp"
#include "pat.hpp"

namespace km {

    constexpr km::PageMemoryTypeLayout GetDefaultPatLayout() {
        enum {
            kEntryWriteBack,
            kEntryWriteThrough,
            kEntryUncachedOverridable,
            kEntryUncached,
            kEntryWriteCombined,
            kEntryWriteProtect,
        };

        return PageMemoryTypeLayout {
            .deferred = kEntryUncachedOverridable,
            .uncached = kEntryUncached,
            .writeCombined = kEntryWriteCombined,
            .writeThrough = kEntryWriteThrough,
            .writeProtect = kEntryWriteProtect,
            .writeBack = kEntryWriteBack,
        };
    }

    PageMemoryTypeLayout SetupPat();

    void SetupMtrrs(x64::MemoryTypeRanges& mtrrs, const km::PageBuilder& pm);
    void WriteMtrrs(const km::PageBuilder& pm);

    void WriteMemoryMap(std::span<const boot::MemoryRegion> memmap);

    void DumpIsrState(const km::IsrContext *context);
    void DumpStackTrace(const km::IsrContext *context);
    void DumpIsrContext(const km::IsrContext *context, stdx::StringView message);
    void InstallExceptionHandlers(SharedIsrTable *ist);

    void InitGlobalAllocator(void *memory, size_t size);
}
