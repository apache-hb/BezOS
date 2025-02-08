#include <gtest/gtest.h>

#include "fs2/vfs.hpp"
#include "std/vector.hpp"

using namespace vfs2;

struct RamFsNode;
struct RamFsMount;
struct RamFs;

struct RamFsNode : public IVfsNode {
    RamFsNode() : IVfsNode() { }
};

struct RamFsFileNode : public RamFsNode {
    stdx::Vector2<std::byte> mData;

    OsStatus read(ReadRequest request, ReadResult *result) override {
        uintptr_t range = (uintptr_t)request.end - (uintptr_t)request.begin;
        size_t size = std::min<size_t>(range, mData.count() - request.offset);
        memcpy(request.begin, mData.data() + request.offset, size);
        result->read = size;
        return OsStatusSuccess;
    }

    OsStatus write(WriteRequest request, WriteResult *result) override {
        uintptr_t range = (uintptr_t)request.end - (uintptr_t)request.begin;
        if ((request.offset + range) > mData.count()) {
            mData.resize(request.offset + range);
        }

        memcpy(mData.data() + request.offset, request.begin, range);
        result->write = range;
        return OsStatusSuccess;
    }
};

struct RamFsFolder : public RamFsNode {
    OsStatus create(IVfsNode **node) override {
        *node = new RamFsFileNode();
        return OsStatusSuccess;
    }
};

struct RamFsMount : public IVfsMount {
    RamFsFolder *mRootNode;

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
    , mRootNode(new RamFsFolder())
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

    std::unique_ptr<IVfsNodeHandle> fd0;

    {
        OsStatus status = file->open(std::out_ptr(fd0));
        ASSERT_EQ(OsStatusSuccess, status);
        ASSERT_NE(fd0, nullptr);
    }

    ASSERT_EQ(fd0->node, file);

    char data[] = "Hello, World!";

    {
        WriteRequest request {
            .begin = std::begin(data),
            .end = std::end(data),
            .offset = 0,
        };

        WriteResult result{};
        OsStatus status = fd0->write(request, &result);
        ASSERT_EQ(OsStatusSuccess, status);
        ASSERT_EQ(result.write, sizeof(data));
    }

    char readback[sizeof(data)];

    {
        ReadRequest request {
            .begin = std::begin(readback),
            .end = std::end(readback),
            .offset = 0,
        };

        ReadResult result{};
        OsStatus status = fd0->read(request, &result);
        ASSERT_EQ(OsStatusSuccess, status);
        ASSERT_EQ(result.read, sizeof(data));

        ASSERT_EQ(std::string_view(readback, sizeof(data)), std::string_view(data, sizeof(data)));
    }
}
