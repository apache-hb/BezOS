#include "fs2/interface.hpp"

using namespace vfs2;

OsStatus FolderMixin::lookup(VfsStringView name, INode **child) {
    if (auto it = mChildren.find(name); it != mChildren.end()) {
        *child = it->second;
        return OsStatusSuccess;
    }

    return OsStatusNotFound;
}

OsStatus FolderMixin::mknode(VfsStringView name, INode *child) {
    if (mChildren.contains(name)) {
        return OsStatusAlreadyExists;
    }

    mChildren.insert({ VfsString(name), child });
    return OsStatusSuccess;
}

OsStatus FolderMixin::rmnode(INode *child) {
    NodeInfo info = child->info();
    if (auto it = mChildren.find(info.name); it != mChildren.end()) {
        mChildren.erase(it);
        return OsStatusSuccess;
    }

    return OsStatusNotFound;
}
