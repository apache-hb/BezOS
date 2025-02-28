#include <gtest/gtest.h>

#include "elf.hpp"

using namespace km;

static constexpr elf::ProgramHeader LoadPhdr(uint64_t offset, uint64_t memsz, uint64_t align) {
    return {
        .type = elf::ProgramHeaderType::eLoad,
        .offset = offset,
        .vaddr = offset,
        .memsz = memsz,
        .align = align,
    };
}

TEST(ElfTest, LoadMemorySizeSimple) {
    elf::ProgramHeader phdrs[] = {
        LoadPhdr(0x1000, 0x1000, 0x1000),
        LoadPhdr(0x2000, 0x1000, 0x1000),
    };

    VirtualRange range;
    OsStatus status = km::detail::LoadMemorySize(phdrs, &range);
    EXPECT_EQ(status, OsStatusSuccess);

    EXPECT_EQ(range.front, (void*)0x1000);
    EXPECT_EQ(range.back, (void*)0x3000);
}

TEST(ElfTest, LoadMemorySizeOffsetAlign) {
    elf::ProgramHeader phdrs[] = {
        LoadPhdr(0x1000, 0x1020, 0x1000),
        LoadPhdr(0x2000, 0x1000, 0x1000),
    };

    VirtualRange range;
    OsStatus status = km::detail::LoadMemorySize(phdrs, &range);
    EXPECT_EQ(status, OsStatusSuccess);

    // back should be 0x4000 because the first phdr spills into the next
    // page and to maintain alignment we need to then shift the following section to the next page.
    EXPECT_EQ(range.front, (void*)0x1000);
    EXPECT_EQ(range.back, (void*)0x4000);
}

TEST(ElfTest, LoadMemorySizeOffsetAlignMany) {
    elf::ProgramHeader phdrs[] = {
        LoadPhdr(0x1000, 0x1020, 0x1000), // actually 2 pages
        LoadPhdr(0x2000, 0x1E00, 0x1000), // 2 pages
        LoadPhdr(0x1F00, 0x4000, 0x1000), // 5 pages
    };

    VirtualRange range;
    OsStatus status = km::detail::LoadMemorySize(phdrs, &range);
    EXPECT_EQ(status, OsStatusSuccess);

    // back should be 0x4000 because the first phdr spills into the next
    // page and to maintain alignment we need to then shift the following section to the next page.
    EXPECT_EQ(range.front, (void*)0x1000);
    EXPECT_EQ(range.back, (void*)0x9000);
}
