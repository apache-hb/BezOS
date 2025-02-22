#include <gtest/gtest.h>

#include "test_memory.hpp"

TEST(MemoryTest, Construct) {
    SystemMemoryTestBody body;
    body.addSegment(0x100000, boot::MemoryRegion::eUsable);
    body.addSegment(0x100000, boot::MemoryRegion::eUsable);

    km::SystemMemory memory = body.make();
}

TEST(MemoryTest, MapKernelAddress) {
    SystemMemoryTestBody body;
    auto s0 = body.addSegment(0x100000, boot::MemoryRegion::eUsable);
    body.addSegment(0x100000, boot::MemoryRegion::eUsable);

    km::SystemMemory memory = body.makeNoAlloc(km::DefaultUserArea());

    void *kptr = memory.map(s0.front, s0.front + 0x1000);
    ASSERT_NE(kptr, nullptr);
    ASSERT_TRUE(body.pm.isHigherHalf(kptr));
}
