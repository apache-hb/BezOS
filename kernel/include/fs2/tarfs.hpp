#pragma once

#include "drivers/block/driver.hpp"

#include "fs2/node.hpp"

namespace vfs2 {
    struct TarPosixHeader {
        char name[100];
        char mode[8];
        char uid[8];
        char gid[8];
        char size[12];
        char mtime[12];
        char checksum[8];
        char typeflag;
        char linkname[100];
        char magic[6];
        char version[2];
        char uname[32];
        char gname[32];
        char devmajor[8];
        char devminor[8];
        char prefix[155];

        size_t getSize() const {
            size_t result = 0;
            for (size_t i = 0; i < 12; i++) {
                result = result * 8 + (size[i] - '0');
            }

            return result;
        }

        VfsNodeType getType() const {
            switch (typeflag) {
            case '0': return VfsNodeType::eFile;
            case '5': return VfsNodeType::eFolder;
            default: return VfsNodeType::eNone;
            }
        }
    };

    static_assert(sizeof(TarPosixHeader) == 500);

    class TarFsNode;
    class TarFsFile;
    class TarFsFolder;
    class TarFsMount;
    class TarFs;

    class TarFsNode : public IVfsNode {
    public:
        TarPosixHeader header;
        size_t offset;
    };

    class TarFsFile : public TarFsNode {

    };

    class TarFsFolder : public TarFsNode {

    };

    BTreeMap<VfsPath, TarFsNode*> ParseTar(km::BlockDevice *media);

    class TarFsMount final : public IVfsMount {
        km::BlockDevice mMedia;
        BTreeMap<VfsPath, TarFsNode*> mEntries;

    public:
        TarFsMount(TarFs *tarfs, km::IBlockDriver *block);

        km::BlockDevice *media() { return &mMedia; }

        OsStatus root(IVfsNode **node) override;
    };

    class TarFsDriver final : public IVfsDriver {
    public:
        constexpr TarFsDriver()
            : IVfsDriver("tarfs")
        { }

        OsStatus mount(IVfsMount **mount) override;
        OsStatus unmount(IVfsMount *mount) override;
    };
}
