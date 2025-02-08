#pragma once

#include "drivers/block/driver.hpp"

#include "fs/mount.hpp"
#include "fs/path.hpp"

namespace vfs {
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

        km::VfsEntryType getType() const {
            switch (typeflag) {
            case '0': return km::VfsEntryType::eFile;
            case '5': return km::VfsEntryType::eFolder;
            default: return km::VfsEntryType::eNone;
            }
        }
    };

    struct TarNode {
        TarPosixHeader header;
        size_t offset;
    };

    static_assert(sizeof(TarPosixHeader) == 500);

    km::BTreeMap<km::VfsPath, TarNode> ParseTar(km::BlockDevice *media);

    class TarFsMount final : public IFileSystemMount {
        km::BlockDevice mMedia;
        km::BTreeMap<km::VfsPath, TarNode> mEntries;

    public:
        TarFsMount(km::IBlockDriver *block)
            : mMedia(block)
            , mEntries(ParseTar(&mMedia))
        { }

        IFileSystem *filesystem() const override;

        sm::SharedPtr<INode> root() const override;

        km::BlockDevice *media() { return &mMedia; }
    };

    class TarFsDriver final : public IFileSystem {
    public:
        TarFsDriver();

        stdx::StringView name() const override {
            using namespace stdx::literals;
            return "tarfs";
        }

        OsStatus mount(IFileSystemMount **mount) override;
    };
}
