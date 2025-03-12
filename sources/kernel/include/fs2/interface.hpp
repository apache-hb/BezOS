#pragma once

#include "fs2/node.hpp"

namespace vfs2 {
    class IFileHandle : public IHandle {
    public:
        virtual ~IFileHandle() = default;

        virtual OsStatus stat(NodeStat *stat) = 0;
    };

    class IFolderHandle : public IHandle {
    public:
        virtual ~IFolderHandle() = default;

        virtual OsStatus lookup(VfsStringView name, INode **child) = 0;
        virtual OsStatus mknode(VfsStringView name, INode *child) = 0;
        virtual OsStatus rmnode(INode *child) = 0;
    };

    class IIdentifyHandle : public IHandle {
    public:
        virtual ~IIdentifyHandle() = default;

        virtual OsStatus identify(OsIdentifyInfo *info) = 0;
        virtual OsStatus interfaces(OsIdentifyInterfaceList *list) = 0;
    };
}
