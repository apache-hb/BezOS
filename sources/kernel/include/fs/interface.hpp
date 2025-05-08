#pragma once

#include "fs/node.hpp"

namespace vfs {
    /// @note All interfaces of @a kOsFileGuid must implement this interface.
    class IFileHandle : public IHandle {
    public:
        virtual ~IFileHandle() = default;

        virtual OsStatus stat(OsFileInfo *info) = 0;
    };

    /// @note All interfaces of @a kOsFolderGuid must implement this interface.
    class IFolderHandle : public IHandle {
    public:
        virtual ~IFolderHandle() = default;

        /// @brief Lookup a child node by name.
        ///
        /// @note This function is expected to be internally synchronized.
        virtual OsStatus lookup(VfsStringView name, sm::RcuSharedPtr<INode> *child) = 0;

        /// @brief Create a child node.
        ///
        /// @note This function is expected to be internally synchronized.
        virtual OsStatus mknode(VfsStringView name, sm::RcuSharedPtr<INode> child) = 0;

        /// @brief Remove a child node.
        ///
        /// @note This function is expected to be internally synchronized.
        virtual OsStatus rmnode(sm::RcuSharedPtr<INode> child) = 0;
    };

    /// @note All interfaces of @a kOsIteratorGuid must implement this interface.
    class IIteratorHandle : public IHandle {
    public:
        virtual ~IIteratorHandle() = default;

        virtual OsStatus next(sm::RcuSharedPtr<INode> *node) = 0;

        OsStatus next(IInvokeContext *context, void *data, size_t size);

        OsStatus invoke(IInvokeContext *context, uint64_t function, void *data, size_t size) override;
    };

    /// @note All interfaces of @a kOsIdentifyGuid must implement this interface.
    class IIdentifyHandle : public IHandle {
    public:
        virtual ~IIdentifyHandle() = default;

        virtual OsStatus identify(OsIdentifyInfo *info) = 0;
        virtual OsStatus interfaces(OsIdentifyInterfaceList *list) = 0;

        OsStatus identify(void *data, size_t size);
        OsStatus interfaces(void *data, size_t size);

        OsStatus invoke(IInvokeContext *context, uint64_t function, void *data, size_t size) override;
    };

    /// @note All interfaces of @a kOsStreamGuid must implement this interface.
    class IStreamHandle : public IHandle {
    public:
        virtual ~IStreamHandle() = default;
    };
}
