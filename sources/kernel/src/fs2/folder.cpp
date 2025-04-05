#include "fs2/folder.hpp"

using namespace vfs2;

OsStatus FolderMixin::lookup(VfsStringView name, sm::RcuSharedPtr<INode> *child) {
    if (auto it = mChildren.find(name); it != mChildren.end()) {
        *child = it->second;
        return OsStatusSuccess;
    }

    return OsStatusNotFound;
}

OsStatus FolderMixin::mknode(sm::RcuWeakPtr<INode> parent, VfsStringView name, sm::RcuSharedPtr<INode> child) {
    if (mChildren.contains(name)) {
        return OsStatusAlreadyExists;
    }

    child->init(parent, VfsString(name), Access::RWX);

    mChildren.insert({ VfsString(name), child });
    return OsStatusSuccess;
}

OsStatus FolderMixin::rmnode(sm::RcuSharedPtr<INode> child) {
    NodeInfo info = child->info();
    if (auto it = mChildren.find(info.name); it != mChildren.end()) {
        mChildren.erase(it);
        return OsStatusSuccess;
    }

    return OsStatusNotFound;
}

OsStatus FolderMixin::next(Iterator *iterator, sm::RcuSharedPtr<INode> *node) {
    if (iterator->name.isEmpty()) {
        iterator->current = mChildren.begin();
    } else {
        iterator->current++;
    }

    if (iterator->current == mChildren.end()) {
        return OsStatusCompleted;
    }

    auto& [name, child] = *iterator->current;
    iterator->name = name;
    *node = child;
    return OsStatusSuccess;
}
