#include "fs/tarfs.hpp"

km::BTreeMap<km::VfsPath, vfs::TarNode> vfs::ParseTar(km::BlockDevice *media) {
    km::BTreeMap<km::VfsPath, vfs::TarNode> entries;

    uint32_t blockSize = media->blockSize();
    uint64_t offset = 0;

    while (true) {
        vfs::TarPosixHeader header;
        size_t read = media->read(offset, &header, sizeof(header));
        if (read != sizeof(header)) {
            break;
        }

        offset += blockSize;

        if (header.name[0] == '\0') {
            break;
        }

        vfs::TarNode node = {
            .header = header,
            .offset = offset,
        };

        size_t size = header.getSize();

        if (size % blockSize) {
            size += blockSize - (size % blockSize);
        }

        offset += size;

        entries.insert({ stdx::StringView(header.name), node });
    }

    return entries;
}
