#include "fs/vfs.hpp"

#include "fs/ramfs.hpp"

#include <utility>

void km::VfsFolder::insert(stdx::String name, std::unique_ptr<VfsEntry> node) {
    using Value = Container::value_type;

    Value value = { std::move(name), std::move(node) };
    nodes.insert(std::move(value));
}

km::VirtualFileSystem::VirtualFileSystem()
    : mRootMount(new vfs::RamFsMount())
    , mRootEntry(mRootMount.get())
{ }

std::tuple<km::VfsEntry*, vfs::IFileSystemMount*> km::VirtualFileSystem::getParentFolder(const VfsPath& path) {
    if (path.segments() == 1)
        return std::make_tuple(&mRootEntry, mRootMount.get());

    VfsEntry *current = &mRootEntry;
    vfs::IFileSystemMount *mount = mRootMount.get();

    for (stdx::StringView segment : path.parent()) {
        VfsFolder& folder = current->folder();
        auto it = folder.find(segment);
        if (it == folder.end()) {
            return { nullptr, nullptr };
        }

        auto& node = it->second;
        if (node->type() != VfsEntryType::eFolder) {
            return { nullptr, nullptr };
        }

        current = node.get();
        mount = node->mount();
    }

    return std::make_tuple(current, mount);
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
        KmStatus status = pnode->mkdir(path.name(), &node);
        if (status != ERROR_SUCCESS) {
            return nullptr;
        }

        std::unique_ptr<VfsEntry> vfsEntry(new VfsEntry(node));
        VfsEntry *ptr = vfsEntry.get();
        folder.insert(stdx::String(path.name()), std::move(vfsEntry));
        return ptr;
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
        KmStatus status = fs->mount(&mount);
        if (status != ERROR_SUCCESS) {
            return nullptr;
        }

        std::unique_ptr<VfsEntry> node(new VfsEntry(mount));
        VfsEntry *ptr = node.get();

        folder.insert(stdx::String(path.name()), std::move(node));

        return ptr;
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
            KmStatus status = pnode->find(path.name(), &node);
            if (status == ERROR_SUCCESS) {
                // If it's in the filesystem, then cache it and return a handle
                // to the newly created entry.
                std::unique_ptr<VfsEntry> vfsEntry(new VfsEntry(node));
                VfsEntry *ptr = vfsEntry.get();

                folder.insert(stdx::String(path.name()), std::move(vfsEntry));
                return new VfsHandle(ptr);
            }

            // If it doesn't exist in either the cache or the filesystem, then
            // create a new file.

            status = pnode->create(path.name(), &node);
            if (status != ERROR_SUCCESS) {
                return nullptr;
            }

            std::unique_ptr<VfsEntry> vfsEntry(new VfsEntry(node));
            VfsEntry *ptr = vfsEntry.get();

            folder.insert(stdx::String(path.name()), std::move(vfsEntry));
            return new VfsHandle(ptr);
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
