#include <gtest/gtest.h>

#include "fs/utils.hpp"
#include "fs/vfs.hpp"
#include "fs/ramfs.hpp"

using namespace vfs;

TEST(Vfs2Test, Construct) {
    vfs::VfsRoot vfs;
}

TEST(Vfs2Test, Mount) {
    vfs::VfsRoot vfs;

    vfs::IVfsMount *mount = nullptr;
    OsStatus status = vfs.addMount(&RamFs::instance(), "Volatile", &mount);
    ASSERT_EQ(OsStatusSuccess, status);
}

TEST(Vfs2Test, CreateFile) {
    vfs::VfsRoot vfs;

    vfs::IVfsMount *mount = nullptr;

    {
        OsStatus status = vfs.addMount(&RamFs::instance(), "Volatile", &mount);
        ASSERT_EQ(OsStatusSuccess, status);
    }

    sm::RcuSharedPtr<INode> file = nullptr;

    {
        OsStatus status = vfs.create(BuildPath("Volatile", "motd.txt"), &file);
        ASSERT_EQ(OsStatusSuccess, status);
        ASSERT_NE(file, nullptr);
    }

    ASSERT_TRUE(vfs::HasInterface(file, kOsFileGuid));
    NodeInfo info = file->info();
    ASSERT_EQ(info.name, "motd.txt");
    ASSERT_EQ(info.mount, mount);

    std::unique_ptr<IFileHandle> fd0;

    {
        OsStatus status = vfs::OpenFileInterface(file, nullptr, 0, std::out_ptr(fd0));
        ASSERT_EQ(OsStatusSuccess, status);
        ASSERT_NE(fd0, nullptr);
    }
}

TEST(Vfs2Test, FileReadWrite) {
    vfs::VfsRoot vfs;

    vfs::IVfsMount *mount = nullptr;

    {
        OsStatus status = vfs.addMount(&RamFs::instance(), "Volatile", &mount);
        ASSERT_EQ(OsStatusSuccess, status);
    }

    sm::RcuSharedPtr<INode> file = nullptr;

    {
        OsStatus status = vfs.create(BuildPath("Volatile", "motd.txt"), &file);
        ASSERT_EQ(OsStatusSuccess, status);
        ASSERT_NE(file, nullptr);
    }


    ASSERT_TRUE(vfs::HasInterface(file, kOsFileGuid));
    NodeInfo info = file->info();
    ASSERT_EQ(info.name, "motd.txt");
    ASSERT_EQ(info.mount, mount);

    std::unique_ptr<IFileHandle> fd0;

    {
        OsStatus status = vfs::OpenFileInterface(file, nullptr, 0, std::out_ptr(fd0));
        ASSERT_EQ(OsStatusSuccess, status);
        ASSERT_NE(fd0, nullptr);
    }

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

TEST(Vfs2Test, MakePath) {
    VfsRoot vfs;

    IVfsMount *mount = nullptr;
    sm::RcuSharedPtr<INode> node = nullptr;

    {
        OsStatus status = vfs.addMount(&RamFs::instance(), "System", &mount);
        ASSERT_EQ(OsStatusSuccess, status);
    }

    {
        auto path = BuildPath("System", "Devices", "CPU", "CPU0", "Firmware");
        OsStatus status = vfs.mkpath(path, &node);
        ASSERT_EQ(OsStatusSuccess, status);
        ASSERT_NE(node, nullptr);
    }

    ASSERT_TRUE(vfs::HasInterface(node, kOsFolderGuid));
    NodeInfo info = node->info();
    ASSERT_EQ(info.name, "Firmware");
    ASSERT_EQ(info.mount, mount);

    sm::RcuSharedPtr<INode> parent = info.parent.lock();
    ASSERT_NE(parent, nullptr);

    NodeInfo pInfo = parent->info();

    ASSERT_EQ(pInfo.name, "CPU0");

    //
    // Ensure that all the parent folders were created.
    //

    sm::RcuSharedPtr<INode> folder = nullptr;
    {
        OsStatus status = vfs.lookup(BuildPath("System", "Devices", "CPU", "CPU0"), &folder);
        ASSERT_EQ(OsStatusSuccess, status);

        ASSERT_TRUE(vfs::HasInterface(folder, kOsFolderGuid));
        NodeInfo info = folder->info();
        ASSERT_EQ(info.name, "CPU0");
        ASSERT_EQ(info.mount, mount);
    }

    {
        OsStatus status = vfs.lookup(BuildPath("System", "Devices", "CPU"), &folder);
        ASSERT_EQ(OsStatusSuccess, status);

        ASSERT_TRUE(vfs::HasInterface(folder, kOsFolderGuid));
        NodeInfo info = folder->info();
        ASSERT_EQ(info.name, "CPU");
        ASSERT_EQ(info.mount, mount);
    }

    {
        OsStatus status = vfs.lookup(BuildPath("System", "Devices"), &folder);
        ASSERT_EQ(OsStatusSuccess, status);

        ASSERT_TRUE(vfs::HasInterface(folder, kOsFolderGuid));
        NodeInfo info = folder->info();
        ASSERT_EQ(info.name, "Devices");
        ASSERT_EQ(info.mount, mount);
    }

    {
        OsStatus status = vfs.lookup(BuildPath("System"), &folder);
        ASSERT_EQ(OsStatusSuccess, status);

        ASSERT_TRUE(vfs::HasInterface(folder, kOsFolderGuid));
        NodeInfo info = folder->info();
        ASSERT_EQ(info.name, "System");
        ASSERT_EQ(info.mount, mount);
    }
}

TEST(Vfs2Test, RemoveFile) {
    VfsRoot vfs;

    IVfsMount *mount = nullptr;
    sm::RcuSharedPtr<INode> node = nullptr;

    {
        OsStatus status = vfs.addMount(&RamFs::instance(), "System", &mount);
        ASSERT_EQ(OsStatusSuccess, status);
    }

    {
        OsStatus status = vfs.create(BuildPath("System", "motd.txt"), &node);
        ASSERT_EQ(OsStatusSuccess, status);
    }

    {
        OsStatus status = vfs.remove(node);
        ASSERT_EQ(OsStatusSuccess, status);
    }

    sm::RcuSharedPtr<INode> lookup = nullptr;
    {
        OsStatus status = vfs.lookup(BuildPath("System", "motd.txt"), &lookup);
        ASSERT_EQ(OsStatusNotFound, status);
        ASSERT_EQ(lookup, nullptr);
    }
}

TEST(Vfs2Test, RemoveFolder) {
    VfsRoot vfs;

    IVfsMount *mount = nullptr;
    sm::RcuSharedPtr<INode> node = nullptr;

    {
        OsStatus status = vfs.addMount(&RamFs::instance(), "System", &mount);
        ASSERT_EQ(OsStatusSuccess, status);
    }

    {
        OsStatus status = vfs.mkpath(BuildPath("System", "Devices", "CPU", "CPU0", "Firmware"), &node);
        ASSERT_EQ(OsStatusSuccess, status);
    }

    {
        OsStatus status = vfs.rmdir(node);
        ASSERT_EQ(OsStatusSuccess, status);
    }

    sm::RcuSharedPtr<INode> lookup = nullptr;
    {
        OsStatus status = vfs.lookup(BuildPath("System", "Devices", "CPU", "CPU0", "Firmware"), &lookup);
        ASSERT_EQ(OsStatusNotFound, status);
        ASSERT_EQ(lookup, nullptr);
    }

    {
        OsStatus status = vfs.lookup(BuildPath("System", "Devices", "CPU", "CPU0"), &lookup);
        ASSERT_EQ(OsStatusSuccess, status);
        ASSERT_NE(lookup, nullptr);
    }
}

static void CreateFile(VfsRoot *vfs, const VfsPath& path, stdx::StringView content) {
    sm::RcuSharedPtr<INode> node = nullptr;
    OsStatus status = vfs->create(path, &node);
    ASSERT_EQ(OsStatusSuccess, status);

    std::unique_ptr<IFileHandle> handle;
    status = vfs::OpenFileInterface(node, nullptr, 0, std::out_ptr(handle));
    ASSERT_EQ(OsStatusSuccess, status);

    WriteRequest request {
        .begin = content.begin(),
        .end = content.end(),
        .offset = 0,
    };

    WriteResult result{};
    status = handle->write(request, &result);
    ASSERT_EQ(OsStatusSuccess, status);

    ASSERT_EQ(result.write, content.count());
}

static VfsString GetName(sm::RcuSharedPtr<INode> node) {
    NodeInfo info = node->info();
    return VfsString(info.name);
}

TEST(Vfs2Test, OpenFolder) {
    VfsRoot vfs;

    IVfsMount *mount = nullptr;
    sm::RcuSharedPtr<INode> node = nullptr;

    {
        OsStatus status = vfs.addMount(&RamFs::instance(), "System", &mount);
        ASSERT_EQ(OsStatusSuccess, status);
    }

    {
        OsStatus status = vfs.mkpath(BuildPath("Test"), &node);
        ASSERT_EQ(OsStatusSuccess, status);
    }

    CreateFile(&vfs, BuildPath("Test", "file1.txt"), "Hello, World!");
    CreateFile(&vfs, BuildPath("Test", "file2.txt"), "Goodbye, World!");
    CreateFile(&vfs, BuildPath("Test", "file3.txt"), "Hello, World!");

    {
        std::unique_ptr<IIteratorHandle> handle;
        OsStatus status = vfs::OpenIteratorInterface(node, nullptr, 0, std::out_ptr(handle));
        ASSERT_EQ(OsStatusSuccess, status);
        ASSERT_NE(handle, nullptr);

        sm::RcuSharedPtr<INode> child = nullptr;
        status = handle->next(&child);
        ASSERT_EQ(OsStatusSuccess, status);
        ASSERT_EQ(GetName(child), "file1.txt");

        status = handle->next(&child);
        ASSERT_EQ(OsStatusSuccess, status);
        ASSERT_EQ(GetName(child), "file2.txt");

        status = handle->next(&child);
        ASSERT_EQ(OsStatusSuccess, status);
        ASSERT_EQ(GetName(child), "file3.txt");

        status = handle->next(&child);
        ASSERT_EQ(OsStatusCompleted, status);
    }
}
