#pragma once

#include "drivers/block/driver.hpp"

#include "fs2/node.hpp"

namespace vfs2 {
    namespace detail {
        static constexpr size_t kTarNameSize = 100;

        /// @brief Converts a tar path to a VfsPath.
        ///
        /// @pre @p path points to a char array of at least 100 bytes.
        ///
        /// @param path The path to convert.
        /// @param result The converted path.
        ///
        /// @return The status of the operation.
        OsStatus ConvertTarPath(const char path[kTarNameSize], VfsPath *result);
    }

    struct TarPosixHeader {
        char name[detail::kTarNameSize];
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

        uint64_t reportedChecksum() const {
            uint64_t result = 0;
            for (size_t i = 0; i < 8; i++) {
                result = result * 8 + (checksum[i] - '0');
            }

            return result;
        }

        uint64_t actualChecksum() const {
            uint64_t result = 0;
            auto sumRange = [&](std::span<const char> range) {
                result += std::accumulate(range.begin(), range.end(), 0);
            };

            //
            // The checksum is computed as the sum of all the bytes in the header.
            //

            sumRange(name);
            sumRange(mode);
            sumRange(uid);
            sumRange(gid);
            sumRange(size);
            sumRange(mtime);

            //
            // During checksum calculation, the checksum field is treated as
            // if it was all spaces.
            //
            std::array<char, 8> space = { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' };
            sumRange(space);

            result += typeflag;

            sumRange(linkname);
            sumRange(magic);
            sumRange(version);
            sumRange(uname);
            sumRange(gname);
            sumRange(devmajor);
            sumRange(devminor);
            sumRange(prefix);

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

    struct TarParseOptions {
        bool ignoreChecksum = false;
    };

    /// @brief Parses a POSIX.1-1988 tar archive.
    ///
    /// @param media The block device containing the tar archive.
    /// @param options The options for parsing the tar archive.
    /// @param result The parsed tar archive.
    ///
    /// @return The status of the operation.
    OsStatus ParseTar(km::BlockDevice *media, TarParseOptions options, BTreeMap<VfsPath, TarPosixHeader> *result);

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

        template<typename... A>
        OsStatus createMount(IVfsMount **mount, A&&... args) {
            TarFsMount *result = new(std::nothrow) TarFsMount(this, std::forward<A>(args)...);
            if (!result) {
                return OsStatusOutOfMemory;
            }

            *mount = result;
            return OsStatusSuccess;
        }
    };
}
