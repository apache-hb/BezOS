#include "memory/tables.hpp"

#include "memory/page_tables.hpp"

using namespace km;

void km::copyHigherHalfMappings(PageTables *tables, const PageTables *source) {
    const x64::PageMapLevel4 *pml4 = source->pml4();
    x64::PageMapLevel4 *self = tables->pml4();

    //
    // Higher half mappings are 256-511, This may not be enough as some of the higher
    // half mappings may not be used yet. In the future we will need to update process
    // page tables when the system pml4 is updated.
    //
    static constexpr size_t kCount = (sizeof(x64::PageMapLevel4) / sizeof(x64::pml4e)) / 2;
    std::copy_n(pml4->entries + kCount, kCount, self->entries + kCount);
}
