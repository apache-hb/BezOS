#include "fs/vfs.hpp"

#include <utility>

void km::VfsFolder::insert(stdx::String name, std::unique_ptr<VfsNode> node) {
    using Value = Container::value_type;

    Value value = { std::move(name), std::move(node) };
    nodes.insert(std::move(value));
}

km::VfsNodeId km::VirtualFileSystem::allocateId() {
    VfsNodeId id = mNextId;
    mNextId = VfsNodeId(std::to_underlying(mNextId) + 1);
    return id;
}

km::VfsFolder *km::VirtualFileSystem::getParentFolder(const VfsPath& path) {
    if (path.segments() == 1)
        return &mRoot;

    VfsFolder *current = &mRoot;

    for (stdx::StringView segment : path.parent()) {
        auto it = current->find(segment);
        if (it == current->end()) {
            return nullptr;
        }

        auto& node = it->second;
        if (node->type() != VfsNodeType::eFolder) {
            return nullptr;
        }

        current = &node->folder();
    }

    return current;
}

km::VfsFile *km::VirtualFileSystem::findFileInCache(VfsNodeId id) {
    if (auto it = mNodeCache.find(id); it != mNodeCache.end()) {
        auto [node, _] = it->second;
        if (node->type() == VfsNodeType::eFile) {
            return &node->file();
        }
    }

    return nullptr;
}

void km::VirtualFileSystem::cacheNode(VfsNodeId id, VfsNode *node) {
    if (auto it = mNodeCache.find(id); it != mNodeCache.end()) {
        auto& [entry, refCount] = it->second;
        KM_CHECK(entry == node, "Node already cached with different pointer.");
        refCount += 1;
    } else {
        mNodeCache.insert({ id, VfsCacheEntry(node, 1) });
    }
}

km::VfsNodeId km::VirtualFileSystem::mkdir(const VfsPath& path) {
    if (VfsFolder *parent = getParentFolder(path)) {
        VfsNodeId id = allocateId();
        std::unique_ptr<VfsNode> node(new VfsNode(id, VfsFolder{}));
        cacheNode(id, node.get());
        parent->insert(stdx::String(path.name()), std::move(node));
        return id;
    }

    return VfsNodeId::eInvalid;
}

km::VfsNodeId km::VirtualFileSystem::open(const VfsPath& path) {
    if (VfsFolder *parent = getParentFolder(path)) {
        auto it = parent->find(path.name());
        if (it != parent->end()) {
            it->second->file().offset = 0;
            cacheNode(it->second->id(), it->second.get());
            return it->second->id();
        }

        VfsNodeId id = allocateId();
        std::unique_ptr<VfsNode> node(new VfsNode(id, VfsFile{}));
        cacheNode(id, node.get());

        parent->insert(stdx::String(path.name()), std::move(node));
        return id;
    }

    return VfsNodeId::eInvalid;
}

void km::VirtualFileSystem::close(VfsNodeId id) {
    if (auto it = mNodeCache.find(id); it != mNodeCache.end()) {
        auto& [node, refCount] = it->second;
        if (refCount == 1) {
            mNodeCache.erase(it);
        } else {
            refCount -= 1;
        }
    }
}

size_t km::VirtualFileSystem::read(VfsNodeId id, void *buffer, size_t size) {
    if (VfsFile *file = findFileInCache(id)) {
        const auto& data = file->data;
        size_t front = file->offset;
        size_t back = std::min(front + size, data.count());

        memcpy(buffer, data.data() + front, back - front);
        file->offset = back;
        return back - front;
    }

    return 0;
}

size_t km::VirtualFileSystem::write(VfsNodeId id, const void *buffer, size_t size) {
    if (VfsFile *file = findFileInCache(id)) {
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
