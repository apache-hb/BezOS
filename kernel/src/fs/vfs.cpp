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
    , mRootEntry(VfsFolder{}, mRootMount.get())
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
        std::unique_ptr<VfsEntry> node(new VfsEntry(VfsFolder{}, mount));
        VfsEntry *ptr = node.get();
        folder.insert(stdx::String(path.name()), std::move(node));
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

km::VfsEntry *km::VirtualFileSystem::open(const VfsPath& path) {
    auto [parent, mount] = getParentFolder(path);
    if (parent != nullptr) {
        VfsFolder& folder = parent->folder();
        auto it = folder.find(path.name());
        if (it != folder.end()) {
            it->second->file().offset = 0;
            return it->second.get();
        }

        std::unique_ptr<VfsEntry> node(new VfsEntry(VfsFile{}, mount));
        VfsEntry *ptr = node.get();

        folder.insert(stdx::String(path.name()), std::move(node));
        return ptr;
    }

    return nullptr;
}

void km::VirtualFileSystem::close(VfsEntry*) {

}

size_t km::VirtualFileSystem::read(VfsEntry *id, void *buffer, size_t size) {
    if (id->type() == VfsEntryType::eFile) {
        VfsFile *file = &id->file();
        const auto& data = file->data;
        size_t front = file->offset;
        size_t back = std::min(front + size, data.count());

        memcpy(buffer, data.data() + front, back - front);
        file->offset = back;
        return back - front;
    }

    return 0;
}

size_t km::VirtualFileSystem::write(VfsEntry *id, const void *buffer, size_t size) {
    if (id->type() == VfsEntryType::eFile) {
        VfsFile *file = &id->file();
        auto& data = file->data;
        size_t front = file->offset;
        size_t back = front + size;

        if (back > data.count()) {
            data.resize(back);
        }

        memcpy(data.data() + front, buffer, size);
        file->offset = back;
        return size;
    }

    return 0;
}
