#include <gtest/gtest.h>

#include "fs2/vfs.hpp"
#include "fs2/ramfs.hpp"

using namespace vfs2;

TEST(Vfs2Test, Construct) {
    vfs2::VfsRoot vfs;
}

TEST(Vfs2Test, Mount) {
    vfs2::VfsRoot vfs;

    vfs2::IVfsMount *mount = nullptr;
    OsStatus status = vfs.addMount(&RamFs::instance(), "Volatile", &mount);
    ASSERT_EQ(OsStatusSuccess, status);
}

TEST(Vfs2Test, CreateFile) {
    vfs2::VfsRoot vfs;

    vfs2::IVfsMount *mount = nullptr;

    {
        OsStatus status = vfs.addMount(&RamFs::instance(), "Volatile", &mount);
        ASSERT_EQ(OsStatusSuccess, status);
    }

    IVfsNode *file = nullptr;

    {
        OsStatus status = vfs.create(BuildPath("Volatile", "motd.txt"), &file);
        ASSERT_EQ(OsStatusSuccess, status);
        ASSERT_NE(file, nullptr);
    }

    ASSERT_TRUE(file->isA(kOsFileGuid));
    ASSERT_EQ(file->name, "motd.txt");
    ASSERT_EQ(file->mount, mount);

    std::unique_ptr<IVfsNodeHandle> fd0;

    {
        OsStatus status = file->open(std::out_ptr(fd0));
        ASSERT_EQ(OsStatusSuccess, status);
        ASSERT_NE(fd0, nullptr);
    }

    ASSERT_EQ(fd0->node, file);
}

TEST(Vfs2Test, FileReadWrite) {
    vfs2::VfsRoot vfs;

    vfs2::IVfsMount *mount = nullptr;

    {
        OsStatus status = vfs.addMount(&RamFs::instance(), "Volatile", &mount);
        ASSERT_EQ(OsStatusSuccess, status);
    }

    IVfsNode *file = nullptr;

    {
        OsStatus status = vfs.create(BuildPath("Volatile", "motd.txt"), &file);
        ASSERT_EQ(OsStatusSuccess, status);
        ASSERT_NE(file, nullptr);
    }

    ASSERT_TRUE(file->isA(kOsFileGuid));
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

TEST(Vfs2Test, MakePath) {
    VfsRoot vfs;

    IVfsMount *mount = nullptr;
    IVfsNode *node = nullptr;

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

    ASSERT_TRUE(node->isA(kOsFolderGuid));
    ASSERT_EQ(node->name, "Firmware");
    ASSERT_EQ(node->mount, mount);

    ASSERT_EQ(node->parent->name, "CPU0");

    //
    // Ensure that all the parent folders were created.
    //

    IVfsNode *folder = nullptr;
    {
        OsStatus status = vfs.lookup(BuildPath("System", "Devices", "CPU", "CPU0"), &folder);
        ASSERT_EQ(OsStatusSuccess, status);

        ASSERT_TRUE(folder->isA(kOsFolderGuid));
        ASSERT_EQ(folder->name, "CPU0");
        ASSERT_EQ(folder->mount, mount);
    }

    {
        OsStatus status = vfs.lookup(BuildPath("System", "Devices", "CPU"), &folder);
        ASSERT_EQ(OsStatusSuccess, status);

        ASSERT_TRUE(folder->isA(kOsFolderGuid));
        ASSERT_EQ(folder->name, "CPU");
        ASSERT_EQ(folder->mount, mount);
    }

    {
        OsStatus status = vfs.lookup(BuildPath("System", "Devices"), &folder);
        ASSERT_EQ(OsStatusSuccess, status);

        ASSERT_TRUE(folder->isA(kOsFolderGuid));
        ASSERT_EQ(folder->name, "Devices");
        ASSERT_EQ(folder->mount, mount);
    }

    {
        OsStatus status = vfs.lookup(BuildPath("System"), &folder);
        ASSERT_EQ(OsStatusSuccess, status);

        ASSERT_TRUE(folder->isA(kOsFolderGuid));
        ASSERT_EQ(folder->name, "System");
        ASSERT_EQ(folder->mount, mount);
    }
}

TEST(Vfs2Test, RemoveFile) {
    VfsRoot vfs;

    IVfsMount *mount = nullptr;
    IVfsNode *node = nullptr;

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

    IVfsNode *lookup = nullptr;
    {
        OsStatus status = vfs.lookup(BuildPath("System", "motd.txt"), &lookup);
        ASSERT_EQ(OsStatusNotFound, status);
        ASSERT_EQ(lookup, nullptr);
    }
}

TEST(Vfs2Test, RemoveFolder) {
    VfsRoot vfs;

    IVfsMount *mount = nullptr;
    IVfsNode *node = nullptr;

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

    IVfsNode *lookup = nullptr;
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
    IVfsNode *node = nullptr;
    OsStatus status = vfs->create(path, &node);
    ASSERT_EQ(OsStatusSuccess, status);

    std::unique_ptr<IVfsNodeHandle> handle;
    status = node->open(std::out_ptr(handle));
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

TEST(Vfs2Test, OpenFolder) {
    VfsRoot vfs;

    IVfsMount *mount = nullptr;
    IVfsNode *node = nullptr;

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
        std::unique_ptr<IVfsNodeHandle> handle;
        OsStatus status = node->opendir(std::out_ptr(handle));
        ASSERT_EQ(OsStatusSuccess, status);
        ASSERT_NE(handle, nullptr);

        VfsString name;
        status = handle->next(&name);
        ASSERT_EQ(OsStatusSuccess, status);
        ASSERT_EQ(name, "file1.txt");

        status = handle->next(&name);
        ASSERT_EQ(OsStatusSuccess, status);
        ASSERT_EQ(name, "file2.txt");

        status = handle->next(&name);
        ASSERT_EQ(OsStatusSuccess, status);
        ASSERT_EQ(name, "file3.txt");

        status = handle->next(&name);
        ASSERT_EQ(OsStatusEndOfFile, status);
    }
}
