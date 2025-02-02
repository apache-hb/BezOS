#include <gtest/gtest.h>

#include "drivers/fs/driver.hpp"
#include "drivers/block/ramblk.hpp"

TEST(GptTest, ParseValid) {
    static constexpr size_t kSize = 0x10000;
    std::unique_ptr<std::byte[]> data{new std::byte[kSize]};
    km::MemoryBlk ramblk(data.get(), kSize);
}
