#include "smbios.hpp"
#include "kernel.hpp"
#include <ktest/ktest.h>

class SmbiosTest : public km::testing::Test {

};

KTEST_F(SmbiosTest, FindNone) {
    km::SmBiosLoadOptions options{};
    km::SmBiosTables tables{};
    km::AddressSpace& memory = km::GetSystemMemory()->pageTables();
    OsStatus status = km::findSmbiosTables(options, memory, &tables);
    KASSERT_EQ(OsStatusNotFound, status);
}

KTEST_F(SmbiosTest, Find32) {
    auto launchInfo = km::testing::getLaunchInfo();
    km::SmBiosLoadOptions options{
        .smbios32Address = launchInfo.smbios32Address,
    };
    km::SmBiosTables tables{};
    km::AddressSpace& memory = km::GetSystemMemory()->pageTables();
    OsStatus status = km::findSmbiosTables(options, memory, &tables);
    KASSERT_EQ(OsStatusSuccess, status);
    KASSERT_NE(nullptr, (void*)tables.entry32);
    KASSERT_EQ(nullptr, (void*)tables.entry64);
    KASSERT_TRUE(tables.entry32Allocation.isValid());
    KASSERT_FALSE(tables.entry64Allocation.isValid());
}

KTEST_F(SmbiosTest, Find64) {
    auto launchInfo = km::testing::getLaunchInfo();
    if (launchInfo.smbios64Address == nullptr) {
        KTEST_SKIP("No SMBIOS 64-bit entry point provided by bootloader.");
    }

    km::SmBiosLoadOptions options{
        .smbios64Address = launchInfo.smbios64Address,
    };
    km::SmBiosTables tables{};
    km::AddressSpace& memory = km::GetSystemMemory()->pageTables();
    OsStatus status = km::findSmbiosTables(options, memory, &tables);
    KASSERT_EQ(OsStatusSuccess, status);
    KASSERT_EQ(nullptr, (void*)tables.entry32);
    KASSERT_NE(nullptr, (void*)tables.entry64);
    KASSERT_TRUE(tables.entry64Allocation.isValid());
    KASSERT_FALSE(tables.entry32Allocation.isValid());
}
