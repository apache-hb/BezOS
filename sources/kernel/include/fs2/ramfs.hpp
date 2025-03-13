#pragma once

#include "fs2/folder.hpp"
#include "fs2/identify.hpp"
#include "fs2/node.hpp"
#include "fs2/query.hpp"
#include "std/shared_spinlock.hpp"
#include "std/vector.hpp"

namespace vfs2 {
    class RamFsNode;
    class RamFsFile;
    class RamFsFolder;
    class RamFsMount;
    class RamFs;

    static constexpr inline OsIdentifyInfo kRamFsInfo {
        .DisplayName = "In-Memory File System",
        .Model = "In-Memory File System",
        .DeviceVendor = "BezOS",
        .FirmwareRevision = "1.0.0",
        .DriverVendor = "BezOS",
        .DriverVersion = OS_VERSION(1, 0, 0),
    };

    class RamFsNode : public INode, public ConstIdentifyMixin<kRamFsInfo> {
        INode *mParent;
        IVfsMount *mMount;
        VfsString mName;

    public:
        RamFsNode(INode *parent, IVfsMount *mount, VfsString name)
            : mParent(parent)
            , mMount(mount)
            , mName(std::move(name))
        { }

        NodeInfo info() override {
            return NodeInfo {
                .name = mName,
                .mount = mMount,
                .parent = mParent,
                .access = Access::RWX,
            };
        }

        void init(INode *parent, VfsString name, Access) override {
            mParent = parent;
            mName = std::move(name);
        }
    };

    class RamFsFile : public RamFsNode {
        stdx::Vector2<std::byte> mData;
        stdx::SharedSpinLock mLock;

    public:
        RamFsFile(INode *parent, IVfsMount *mount, VfsString name)
            : RamFsNode(parent, mount, std::move(name))
        { }

        OsStatus query(sm::uuid uuid, const void *, size_t, IHandle **handle) override;
        OsStatus interfaces(void *data, size_t size);

        OsStatus read(ReadRequest request, ReadResult *result);
        OsStatus write(WriteRequest request, WriteResult *result);
        OsStatus stat(NodeStat *stat);
    };

    class RamFsFolder : public RamFsNode, public FolderMixin {
    public:
        RamFsFolder(INode *parent, IVfsMount *mount, VfsString name)
            : RamFsNode(parent, mount, std::move(name))
        { }

        OsStatus query(sm::uuid uuid, const void *data, size_t size, IHandle **handle) override;
        OsStatus interfaces(void *data, size_t size);

        using FolderMixin::lookup;
        using FolderMixin::mknode;
        using FolderMixin::rmnode;
    };

    class RamFsMount : public IVfsMount {
        RamFsFolder *mRootNode;

    public:
        RamFsMount(RamFs *fs);

        OsStatus mkdir(INode *parent, VfsStringView name, const void *data, size_t size, INode **node) override;
        OsStatus create(INode *parent, VfsStringView name, const void *data, size_t size, INode **node) override;

        OsStatus root(INode **node) override;
    };

    class RamFs : public IVfsDriver {
        constexpr RamFs() : IVfsDriver("ramfs") { }

    public:
        OsStatus mount(IVfsMount **mount) override;
        OsStatus unmount(IVfsMount *mount) override;

        static RamFs& instance();
    };
}
