#pragma once

#include "std/vector.hpp"

#include "fs/mount.hpp"

namespace vfs {
    class RamFsNode;
    class RamFsMount;
    class RamFs;

    class RamFsNode : public INode {
        RamFsMount *mMount;

    protected:
        RamFsMount *mount() const { return mMount; }

    public:
        RamFsNode(RamFsMount *mount);

        IFileSystemMount *owner() const override;
    };

    class RamFsFile final : public RamFsNode {
        stdx::Vector2<std::byte> mData;
    public:
        RamFsFile(RamFsMount *mount);

        km::VfsEntryType type() const override { return km::VfsEntryType::eFile; }

        OsStatus read(ReadRequest request, ReadResult *result) override;
        OsStatus write(WriteRequest request, WriteResult *result) override;
    };

    class RamFsFolder final : public RamFsNode {
    public:
        RamFsFolder(RamFsMount *mount);

        km::VfsEntryType type() const override { return km::VfsEntryType::eFolder; }

        OsStatus create(stdx::StringView name, INode **node) override;

        OsStatus mkdir(stdx::StringView name, INode **node) override;
    };

    class RamFsMount final : public IFileSystemMount {
        sm::SharedPtr<RamFsNode> mRoot;
    public:
        RamFsMount();

        IFileSystem *filesystem() const override;
        sm::SharedPtr<INode> root() const override;
    };

    class RamFs final : public IFileSystem {
    public:
        stdx::StringView name() const override;
        OsStatus mount(IFileSystemMount **mount) override;

        static RamFs& get();
    };
}
