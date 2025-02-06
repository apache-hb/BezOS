#include <gtest/gtest.h>

#include "fs/vfs.hpp"
#include "fs/ramfs.hpp"

void DefaultInternalLog(absl::LogSeverity, const char *, int, std::string_view message) {
    std::cout << "Internal log: " << message << std::endl;
}

constinit absl::base_internal::AtomicHook<absl::raw_log_internal::InternalLogFunction>
    absl::raw_log_internal::internal_log_function(DefaultInternalLog);

using namespace stdx::literals;

TEST(VfsTest, Construct) {
    km::VirtualFileSystem vfs;
}

TEST(VfsText, CreateFolder) {
    km::VirtualFileSystem vfs;
    auto entry = vfs.mkdir("/Users"_sv);

    ASSERT_NE(entry, nullptr);
    ASSERT_EQ(entry->type(), km::VfsEntryType::eFolder);
    ASSERT_NE(entry->node(), nullptr);
    ASSERT_NE(entry->mount(), nullptr);
}

TEST(VfsTest, CreateFile) {
    km::VirtualFileSystem vfs;
    vfs.mkdir("/"_sv);
    vfs.mkdir("/Users"_sv);

    vfs.mkdir("/Users/Guest"_sv);
    std::unique_ptr<km::VfsHandle> file{vfs.open("/Users/Guest/motd.txt"_sv)};

    ASSERT_NE(file, nullptr);

    size_t write = 0;
    OsStatus status = file->write("Welcome.\n", 9, &write);
    ASSERT_EQ(status, OsStatusSuccess);
    ASSERT_EQ(write, 9);

    std::unique_ptr<km::VfsHandle> read{vfs.open("/Users/Guest/motd.txt"_sv)};

    ASSERT_NE(read, nullptr);

    char buffer[256];
    size_t readSize = 0;
    status = read->read(buffer, 256, &readSize);
    ASSERT_EQ(status, OsStatusSuccess);
    ASSERT_EQ(readSize, 9);

    buffer[readSize] = '\0';
    ASSERT_STREQ(buffer, "Welcome.\n");
}

TEST(VfsTest, CreateMount) {
    km::VirtualFileSystem vfs;

    auto *system = vfs.mkdir("/System"_sv);
    vfs.mkdir("/System/Config"_sv);

    auto *entry = vfs.mount("/Init"_sv, &vfs::RamFs::get());
    auto *subdir = vfs.mkdir("/Init/DriverConfig"_sv);

    ASSERT_NE(entry, nullptr);
    ASSERT_NE(subdir, nullptr);

    ASSERT_NE(system->mount(), nullptr);
    ASSERT_NE(entry->mount(), nullptr);

    ASSERT_NE(system->mount(), entry->mount());
    ASSERT_EQ(entry->mount(), subdir->mount());
}

TEST(VfsTest, DeleteFile) {
    km::VirtualFileSystem vfs;

    vfs.mkdir("/System"_sv);
    auto *dir = vfs.mkdir("/System/Config"_sv);
    ASSERT_NE(dir, nullptr);

    auto *entry = vfs.open("/System/Config/Settings.ini"_sv);
    ASSERT_NE(entry, nullptr);
    vfs.close(entry);

    vfs.unlink("/System/Config/Settings.ini"_sv);
}
