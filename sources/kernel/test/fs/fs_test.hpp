#pragma once

#include "drivers/block/driver.hpp"
#include <filesystem>

class FileBlk final : public km::IBlockDriver {
	uint32_t mBlockSize;

	int mFileHandle = 0;
	void *mMemoryMap = nullptr;
	size_t mMemorySize = 0;

	void cleanup();

	km::BlockDeviceStatus readImpl(uint64_t block, void *buffer, size_t count) override;

	km::BlockDeviceStatus writeImpl(uint64_t, const void *, size_t) override;

public:
    UTIL_NOCOPY(FileBlk);
    UTIL_NOMOVE(FileBlk);

	FileBlk(const std::filesystem::path& path, uint32_t blockSize);
	~FileBlk();

	km::BlockDeviceCapability capability() const override;
};
