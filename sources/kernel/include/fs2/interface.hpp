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

    class IIteratorHandle : public IHandle {
    public:
        virtual ~IIteratorHandle() = default;

        virtual OsStatus next(INode **node) = 0;

        OsStatus next(void *data, size_t size);

        OsStatus invoke(uint64_t function, void *data, size_t size) override;
    };

    class IIdentifyHandle : public IHandle {
    public:
        virtual ~IIdentifyHandle() = default;

        virtual OsStatus identify(OsIdentifyInfo *info) = 0;
        virtual OsStatus interfaces(OsIdentifyInterfaceList *list) = 0;

        OsStatus identify(void *data, size_t size);
        OsStatus interfaces(void *data, size_t size);

        OsStatus invoke(uint64_t function, void *data, size_t size) override;
    };

    class IStreamHandle : public IHandle {
    public:
        virtual ~IStreamHandle() = default;
    };
}
