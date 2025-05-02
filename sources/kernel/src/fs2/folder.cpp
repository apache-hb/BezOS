#include "fs2/folder.hpp"

using namespace vfs2;

OsStatus FolderMixin::lookup(VfsStringView name, sm::RcuSharedPtr<INode> *child) {
    stdx::SharedLock guard(mLock);

    if (auto it = mChildren.find(name); it != mChildren.end()) {
        *child = it->second;
        return OsStatusSuccess;
    }

    return OsStatusNotFound;
}

OsStatus FolderMixin::mknode(sm::RcuWeakPtr<INode> parent, VfsStringView name, sm::RcuSharedPtr<INode> child) {
    stdx::UniqueLock guard(mLock);

    if (mChildren.contains(name)) {
        return OsStatusAlreadyExists;
    }

    child->init(parent, VfsString(name), sys::NodeAccess::RWX);

    mChildren.insert({ VfsString(name), child });

    mGeneration += 1;

    return OsStatusSuccess;
}

OsStatus FolderMixin::rmnode(sm::RcuSharedPtr<INode> child) {
    stdx::UniqueLock guard(mLock);

    NodeInfo info = child->info();
    if (auto it = mChildren.find(info.name); it != mChildren.end()) {
        mChildren.erase(it);

        mGeneration += 1;

        return OsStatusSuccess;
    }

    return OsStatusNotFound;
}

OsStatus FolderMixin::next(Iterator *iterator, sm::RcuSharedPtr<INode> *node) {
    stdx::UniqueLock guard(mLock);

    if (iterator->name.isEmpty()) {
        iterator->current = mChildren.begin();
        iterator->generation = mGeneration;
    } else if (iterator->generation != mGeneration) {
        //
        // If the iterator has been invalidated, we use the cached last node to
        // find the next node.
        //
        iterator->current = mChildren.upper_bound(iterator->name);
        iterator->generation = mGeneration;
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
