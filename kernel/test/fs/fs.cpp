#include <gtest/gtest.h>

#include "fs2/vfs.hpp"

struct RamFsNode;
struct RamFsMount;
struct RamFs;

struct RamFsMount : public vfs2::IVfsMount {
    RamFsMount(RamFs *fs);
};

struct RamFs : public vfs2::IVfsDriver {
    constexpr RamFs() : IVfsDriver("ramfs") { }

    OsStatus mount(vfs2::IVfsMount **mount) override {
        *mount = new RamFsMount(this);
        return OsStatusSuccess;
    }

    OsStatus unmount(vfs2::IVfsMount *mount) override {
        delete mount;
        return OsStatusSuccess;
    }
};

static constinit RamFs gRamFs{};

RamFsMount::RamFsMount(RamFs *fs)
    : IVfsMount(fs)
{ }

TEST(Vfs2Test, Construct) {
    vfs2::RootVfs vfs;
}

TEST(Vfs2Test, Mount) {
    vfs2::RootVfs vfs;

    vfs2::IVfsMount *mount = nullptr;
    OsStatus status = vfs.addMount(&gRamFs, "/System/Init", &mount);
    ASSERT_EQ(OsStatusSuccess, status);
}
