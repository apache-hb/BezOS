#include <gtest/gtest.h>

#include "fs2/vfs.hpp"

using namespace vfs2;

struct RamFsNode;
struct RamFsMount;
struct RamFs;

struct RamFsNode : public IVfsNode {

};

struct RamFsMount : public IVfsMount {
    RamFsNode *mRootNode;

    RamFsMount(RamFs *fs);

    OsStatus root(IVfsNode **node) override {
        *node = mRootNode;
        return OsStatusSuccess;
    }
};

struct RamFs : public IVfsDriver {
    constexpr RamFs() : IVfsDriver("ramfs") { }

    OsStatus mount(IVfsMount **mount) override {
        *mount = new RamFsMount(this);
        return OsStatusSuccess;
    }

    OsStatus unmount(IVfsMount *mount) override {
        delete mount;
        return OsStatusSuccess;
    }
};

static constinit RamFs gRamFs{};

RamFsMount::RamFsMount(RamFs *fs)
    : IVfsMount(fs)
    , mRootNode(new RamFsNode())
{
    mRootNode->mount = this;
    mRootNode->type = VfsNodeType::eFolder;
}

TEST(Vfs2Test, Construct) {
    vfs2::VfsRoot vfs;
}

TEST(Vfs2Test, Mount) {
    vfs2::VfsRoot vfs;

    vfs2::IVfsMount *mount = nullptr;
    OsStatus status = vfs.addMount(&gRamFs, "Volatile", &mount);
    ASSERT_EQ(OsStatusSuccess, status);
}

TEST(Vfs2Test, CreateFile) {
    vfs2::VfsRoot vfs;

    vfs2::IVfsMount *mount = nullptr;

    {
        OsStatus status = vfs.addMount(&gRamFs, "Volatile", &mount);
        ASSERT_EQ(OsStatusSuccess, status);
    }

    IVfsNode *file = nullptr;

    {
        OsStatus status = vfs.createFile(BuildPath("Volatile", "motd.txt"), &file);
        ASSERT_EQ(OsStatusSuccess, status);
        ASSERT_NE(file, nullptr);
    }

    ASSERT_EQ(file->type, VfsNodeType::eFile);
    ASSERT_EQ(file->name, "motd.txt");
    ASSERT_EQ(file->mount, mount);

    std::unique_ptr<IVfsHandle> fd0;

    {
        OsStatus status = file->open(std::out_ptr(fd0));
        ASSERT_EQ(OsStatusSuccess, status);
        ASSERT_NE(fd0, nullptr);
    }

    ASSERT_EQ(fd0->node, file);
}
