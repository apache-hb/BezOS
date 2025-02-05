#pragma once

#include "std/string.hpp"
#include "std/vector.hpp"

#include "fs/mount.hpp"

namespace vfs {
    class RamFsNode;
    class RamFsMount;
    class RamFs;

    class RamFsNode : public EmptyNode {
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

        KmStatus read(ReadRequest request, ReadResult *result) override;
        KmStatus write(WriteRequest request, WriteResult *result) override;
    };

    class RamFsFolder final : public RamFsNode {
        km::BTreeMap<stdx::String, std::unique_ptr<RamFsNode>, std::less<>> mChildren;

    public:
        RamFsFolder(RamFsMount *mount);

        km::VfsEntryType type() const override { return km::VfsEntryType::eFolder; }

        KmStatus create(stdx::StringView name, INode **node) override;
        KmStatus remove(INode *node) override;

        KmStatus mkdir(stdx::StringView name, INode **node) override;
        KmStatus rmdir(stdx::StringView name) override;

        KmStatus find(stdx::StringView name, INode **node) override;
    };

    class RamFsMount final : public IFileSystemMount {
        std::unique_ptr<RamFsNode> mRoot;
    public:
        RamFsMount();

        IFileSystem *filesystem() const override;
        INode *root() const override;
    };

    class RamFs final : public IFileSystem {
    public:
        stdx::StringView name() const override;
        KmStatus mount(IFileSystemMount **mount) override;

        static RamFs& get();
    };
}
