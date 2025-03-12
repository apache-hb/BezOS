#pragma once

#include "drivers/block/driver.hpp"

#include "fs2/interface.hpp"
#include "fs2/node.hpp"
#include "std/shared.hpp"

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

        template<typename T>
        T TarNumber(std::span<const char> text) {
            T result = 0;
            for (size_t i = 0; i < 12; i++) {
                if (text[i] < '0' || text[i] > '7') {
                    break;
                }

                result = result * 8 + (text[i] - '0');
            }
            return result;
        }
    }

    struct TarPosixHeader {
        static constexpr std::array<char, 6> kMagic = { 'u', 's', 't', 'a', 'r', ' ' };

        char name[detail::kTarNameSize];
        char mode[8];
        char uid[8];
        char gid[8];
        char size[12];
        char mtime[12];
        char checksum[8];
        char typeflag;
        char linkname[100];
        std::array<char, 6> magic;
        char version[2];
        char uname[32];
        char gname[32];
        char devmajor[8];
        char devminor[8];
        char prefix[155];

        size_t getSize() const {
            return detail::TarNumber<size_t>(size);
        }

        uint64_t reportedChecksum() const {
            return detail::TarNumber<uint64_t>(checksum);
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

    struct TarParseOptions {
        bool ignoreChecksum = false;
    };

    struct TarEntry {
        TarPosixHeader header;
        uint64_t offset;
    };

    static_assert(sizeof(TarPosixHeader) == 500);

    class TarFsNode;
    class TarFsFile;
    class TarFsFolder;
    class TarFsMount;
    class TarFs;

    class TarFsNode : public INode {
        INode *mParent;
        TarPosixHeader mHeader;
        uint64_t mOffset;
        TarFsMount *mMount;
        Access mAccess = Access::R;

    public:
        TarFsNode(TarEntry entry, INode *parent, TarFsMount *mount)
            : mParent(parent)
            , mHeader(entry.header)
            , mOffset(entry.offset)
            , mMount(mount)
        { }

        OsStatus query(sm::uuid uuid, const void *data, size_t size, IHandle **handle) override;
        NodeInfo info() override;
        void init(INode *parent, VfsString name, Access access) override;

        OsStatus stat(NodeStat *result);
        OsStatus read(ReadRequest request, ReadResult *result);
    };

    class TarFsFolder : public TarFsNode, public FolderMixin {
    public:
        TarFsFolder(TarEntry entry, INode *parent, TarFsMount *mount)
            : TarFsNode(entry, parent, mount)
        { }

        OsStatus query(sm::uuid uuid, const void *data, size_t size, IHandle **handle) override;

        using FolderMixin::lookup;
        using FolderMixin::mknode;
    };

    /// @brief Parses a POSIX.1-1988 tar archive.
    ///
    /// @param media The block device containing the tar archive.
    /// @param options The options for parsing the tar archive.
    /// @param result The parsed tar archive.
    ///
    /// @return The status of the operation.
    OsStatus ParseTar(km::BlockDevice *media, TarParseOptions options, sm::BTreeMap<VfsPath, TarEntry> *result);

    class TarFsMount final : public IVfsMount {
        sm::SharedPtr<km::IBlockDriver> mBlock;
        km::BlockDevice mMedia;
        FolderNode *mRootNode;

        OsStatus walk(const VfsPath& path, INode **folder);

    public:
        TarFsMount(TarFs *tarfs, sm::SharedPtr<km::IBlockDriver> block);

        km::BlockDevice *media() { return &mMedia; }

        OsStatus root(INode **node) override;
    };

    class TarFs final : public IVfsDriver {
    public:
        constexpr TarFs()
            : IVfsDriver("tarfs")
        { }

        OsStatus mount(IVfsMount **mount) override;
        OsStatus unmount(IVfsMount *mount) override;

        OsStatus createMount(IVfsMount **mount, sm::SharedPtr<km::IBlockDriver> block);

        static TarFs& instance();
    };
}
