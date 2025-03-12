#pragma once

#include "fs2/node.hpp"
#include "std/shared_spinlock.hpp"
#include "std/vector.hpp"

namespace vfs2 {
    class RamFsNode;
    class RamFsFile;
    class RamFsFolder;
    class RamFsMount;
    class RamFs;

    class RamFsNode : public IVfsNode {
    public:
        RamFsNode(VfsNodeType type)
            : IVfsNode(type)
        { }
    };

    class RamFsFile : public RamFsNode {
        stdx::Vector2<std::byte> mData;
        stdx::SharedSpinLock mLock;

    public:
        RamFsFile()
            : RamFsNode(VfsNodeType::eFile)
        { }

        OsStatus read(ReadRequest request, ReadResult *result) override;
        OsStatus write(WriteRequest request, WriteResult *result) override;
        OsStatus stat(NodeStat *stat) override;
    };

    class RamFsFolder : public RamFsNode {
    public:
        RamFsFolder()
            : RamFsNode(VfsNodeType::eFolder)
        { }

        OsStatus create(IVfsNode **node) override;
        OsStatus remove(IVfsNode *node) override;
        OsStatus rmdir(IVfsNode* node) override;
        OsStatus mkdir(IVfsNode **node) override;
    };

    class RamFsMount : public IVfsMount {
        RamFsFolder *mRootNode;

    public:
        RamFsMount(RamFs *fs);

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
