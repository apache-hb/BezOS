#include "fs/vfs.hpp"

#include "fs/ramfs.hpp"

#include <utility>

using namespace stdx::literals;

km::VfsEntry *km::VfsFolder::insert(stdx::String name, std::unique_ptr<VfsEntry> node) {
    using Value = Container::value_type;

    auto [iter, _] = nodes.insert(Value { std::move(name), std::move(node) });

    return iter->second.get();
}

km::VirtualFileSystem::VirtualFileSystem()
    : mRootMount(new vfs::RamFsMount())
    , mRootEntry(mRootMount)
{
    mMounts.insert(ValueType{ VfsPath("/"_sv), std::unique_ptr<vfs::IFileSystemMount>(mRootMount) });
}

std::tuple<km::VfsEntry*, vfs::IFileSystemMount*> km::VirtualFileSystem::getParentFolder(const VfsPath& path) {
    if (path.segments() == 1)
        return std::make_tuple(&mRootEntry, mRootMount);

    VfsEntry *current = &mRootEntry;

    for (stdx::StringView segment : path.parent()) {
        VfsFolder& folder = current->folder();
        auto it = folder.find(segment);
        if (it == folder.end()) {
            return { nullptr, nullptr };
        }

        auto& node = it->second;
        if (node->type() != VfsEntryType::eFolder && node->type() != VfsEntryType::eMount) {
            return { nullptr, nullptr };
        }

        current = node.get();
    }

    return std::make_tuple(current, current->mount());
}

km::VfsEntry *km::VirtualFileSystem::mkdir(const VfsPath& path) {
    auto [parent, mount] = getParentFolder(path);
    if (parent != nullptr) {
        VfsFolder& folder = parent->folder();
        auto it = folder.find(path.name());
        if (it != folder.end()) {
            return nullptr;
        }

        vfs::INode *pnode = parent->node();

        vfs::INode *node = nullptr;
        OsStatus status = pnode->mkdir(path.name(), &node);
        if (status != ERROR_SUCCESS) {
            return nullptr;
        }

        return folder.insert(stdx::String(path.name()), std::unique_ptr<VfsEntry>(new VfsEntry(node)));
    }

    return nullptr;
}

km::VfsEntry *km::VirtualFileSystem::mount(const VfsPath& path, vfs::IFileSystem *fs) {
    auto [parent, mount] = getParentFolder(path);
    if (parent != nullptr) {
        VfsFolder& folder = parent->folder();
        auto it = folder.find(path.name());
        if (it != folder.end()) {
            return nullptr;
        }

        vfs::IFileSystemMount *mount = nullptr;
        OsStatus status = fs->mount(&mount);
        if (status != ERROR_SUCCESS) {
            return nullptr;
        }

        mMounts.insert(ValueType{ path, std::unique_ptr<vfs::IFileSystemMount>(mount) });

        return folder.insert(stdx::String(path.name()), std::unique_ptr<VfsEntry>(new VfsEntry(mount)));
    }

    return nullptr;
}

km::VfsHandle *km::VirtualFileSystem::open(const VfsPath& path) {
    auto [parent, mount] = getParentFolder(path);

    // If the parent folder doesnt exist, then return.
    if (parent != nullptr) {
        VfsFolder& folder = parent->folder();
        auto it = folder.find(path.name());

        // Find look in the cache to see if the file exists.
        if (it == folder.end()) {
            vfs::INode *pnode = parent->node();

            // If it doesn't exist then try to find it in the filesystem.
            vfs::INode *node = nullptr;
            OsStatus status = pnode->find(path.name(), &node);
            if (status == ERROR_SUCCESS) {
                // If it's in the filesystem, then cache it and return a handle
                // to the newly created entry.
                return new VfsHandle(folder.insert(stdx::String(path.name()), std::unique_ptr<VfsEntry>(new VfsEntry(node))));
            }

            // If it doesn't exist in either the cache or the filesystem, then
            // create a new file.

            status = pnode->create(path.name(), &node);
            if (status != ERROR_SUCCESS) {
                return nullptr;
            }

            return new VfsHandle(folder.insert(stdx::String(path.name()), std::unique_ptr<VfsEntry>(new VfsEntry(node))));
        }

        if (it->second->type() != VfsEntryType::eFile) {
            return nullptr;
        }

        return new VfsHandle(it->second.get());
    }

    return nullptr;
}

void km::VirtualFileSystem::close(VfsHandle*) {

}
