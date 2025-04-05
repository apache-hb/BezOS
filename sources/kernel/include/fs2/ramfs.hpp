#pragma once

#include "fs2/device.hpp"
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

    class RamFsNode : public BasicNode, public ConstIdentifyMixin<kRamFsInfo> {
    public:
        RamFsNode(sm::RcuWeakPtr<INode> parent, IVfsMount *mount, VfsString name)
            : BasicNode(parent, mount, std::move(name))
        { }
    };

    class RamFsFile : public RamFsNode {
        stdx::Vector2<std::byte> mData;
        stdx::SharedSpinLock mLock;

    public:
        RamFsFile(sm::RcuWeakPtr<INode> parent, IVfsMount *mount, VfsString name)
            : RamFsNode(parent, mount, std::move(name))
        { }

        OsStatus query(sm::uuid uuid, const void *, size_t, IHandle **handle) override;
        OsStatus interfaces(OsIdentifyInterfaceList *list);

        OsStatus read(ReadRequest request, ReadResult *result);
        OsStatus write(WriteRequest request, WriteResult *result);
        OsStatus stat(NodeStat *stat);
    };

    class RamFsFolder : public RamFsNode, public FolderMixin {
    public:
        RamFsFolder(sm::RcuWeakPtr<INode> parent, IVfsMount *mount, VfsString name)
            : RamFsNode(parent, mount, std::move(name))
        { }

        OsStatus query(sm::uuid uuid, const void *data, size_t size, IHandle **handle) override;
        OsStatus interfaces(OsIdentifyInterfaceList *list);

        using FolderMixin::lookup;
        using FolderMixin::mknode;
        using FolderMixin::rmnode;
    };

    class RamFsMount : public IVfsMount {
        sm::RcuSharedPtr<RamFsFolder> mRootNode;

    public:
        RamFsMount(RamFs *fs, sm::RcuDomain *domain);

        OsStatus mkdir(sm::RcuSharedPtr<INode> parent, VfsStringView name, const void *data, size_t size, sm::RcuSharedPtr<INode> *node) override;
        OsStatus create(sm::RcuSharedPtr<INode> parent, VfsStringView name, const void *data, size_t size, sm::RcuSharedPtr<INode> *node) override;

        OsStatus root(sm::RcuSharedPtr<INode> *node) override;
    };

    class RamFs : public IVfsDriver {
        constexpr RamFs() : IVfsDriver("ramfs") { }

    public:
        OsStatus mount(sm::RcuDomain *domain, IVfsMount **mount) override;
        OsStatus unmount(IVfsMount *mount) override;

        static RamFs& instance();
    };
}
