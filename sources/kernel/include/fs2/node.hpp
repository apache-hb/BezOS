#pragma once

#include <bezos/status.h>
#include <bezos/subsystem/identify.h>
#include <bezos/subsystem/fs.h>

#include "fs2/path.hpp"

#include "std/string_view.hpp"

#include "std/vector.hpp"
#include "util/absl.hpp"

#include "util/util.hpp"
#include "util/uuid.hpp"

namespace km {
    class CallContext;
}

/// @brief Virtual File System.
///
/// This VFS derives from the original SunOS vnode design with some modifications.
///
/// @cite SunVNodes
namespace vfs2 {
    class VfsPath;

    class IVfsNodeHandle;

    class VfsFolderHandle;
    class IVfsNode;

    struct IVfsMount;
    struct IVfsDriver;

    enum class VfsNodeId : uint64_t { };

    using VfsCreateHandle = OsStatus(*)(IVfsNode*, IVfsNodeHandle**);

    struct ReadRequest {
        void *begin;
        void *end;
        uint64_t offset;
        OsInstant timeout;

        intptr_t size() const { return (std::byte*)end - (std::byte*)begin; }
    };

    struct ReadResult {
        uint64_t read;
    };

    struct WriteRequest {
        const void *begin;
        const void *end;
        uint64_t offset;
        OsInstant timeout;

        uintptr_t size() const { return (std::byte*)end - (std::byte*)begin; }
    };

    struct WriteResult {
        uint64_t write;
    };

    enum class VfsNodeType {
        eNone,
        eFile,
        eFolder,
    };

    struct VfsNodeStat {
        uint64_t size;
    };

    class IVfsNodeHandle {
    public:
        virtual ~IVfsNodeHandle() = default;

        IVfsNodeHandle(IVfsNode *it)
            : node(it)
        { }

        IVfsNode *node;

        virtual OsStatus read(ReadRequest request, ReadResult *result);
        virtual OsStatus write(WriteRequest request, WriteResult *result);
        virtual OsStatus stat(VfsNodeStat *stat);
        virtual OsStatus call(uint64_t, void*, size_t) { return OsStatusNotSupported; }
        virtual OsStatus next(VfsString *) { return OsStatusNotSupported; }
    };

    template<typename T> requires (std::is_trivial_v<T>)
    OsStatus ReadObject(IVfsNodeHandle *handle, T *object, uint64_t offset) {
        ReadRequest request {
            .begin = object,
            .end = (std::byte*)object + sizeof(T),
            .offset = offset,
        };

        ReadResult result{};
        if (OsStatus status = handle->read(request, &result)) {
            return status;
        }

        if (result.read != sizeof(T)) {
            return OsStatusEndOfFile;
        }

        return OsStatusSuccess;
    }

    template<typename T> requires (std::is_trivial_v<T>)
    OsStatus ReadArray(IVfsNodeHandle *handle, T *array, size_t count, uint64_t offset) {
        ReadRequest request {
            .begin = array,
            .end = (std::byte*)array + sizeof(T) * count,
            .offset = offset,
        };

        ReadResult result{};
        if (OsStatus status = handle->read(request, &result)) {
            return status;
        }

        if (result.read != sizeof(T) * count) {
            return OsStatusEndOfFile;
        }

        return OsStatusSuccess;
    }

    class VfsFolderHandle : public IVfsNodeHandle {
        size_t mIndex = 0;
        stdx::Vector2<VfsString> mEntries;

    public:
        VfsFolderHandle(IVfsNode *node);

        OsStatus next(VfsString *name) override;
    };

    class IVfsNode {
        friend class VfsIdentifyHandle;

    public:
        using FolderContainer = sm::BTreeMap<VfsString, IVfsNode*, std::less<>>;

        IVfsNode() : IVfsNode(VfsNodeType::eNone) { }

        IVfsNode(VfsNodeType type);

        virtual ~IVfsNode();

        UTIL_NOCOPY(IVfsNode);
        UTIL_NOMOVE(IVfsNode);

        /// @brief The name of the entry.
        VfsString name;

        /// @brief The parent directory of the entry, nullptr if root.
        IVfsNode *parent;

        /// @brief The mount that this node is part of.
        IVfsMount *mount;

    protected:
        /// @brief If this is a directory, these are all the entries.
        FolderContainer mChildren;

        /// @brief The type of the entry.
        VfsNodeType mType;

        OsIdentifyInfo mIdentifyInfo;

        sm::FlatHashMap<sm::uuid, VfsCreateHandle> mInterfaces;

        template<std::derived_from<IVfsNodeHandle> T>
        void addInterface(sm::uuid uuid) {
            VfsCreateHandle callback = [](IVfsNode *node, IVfsNodeHandle **handle) -> OsStatus {
                T *objHandle = new (std::nothrow) T(node);
                if (objHandle == nullptr) {
                    return OsStatusOutOfMemory;
                }

                *handle = objHandle;
                return OsStatusSuccess;
            };

            auto [_, ok] = mInterfaces.insert({ uuid, callback });
            KM_CHECK(ok, "Failed to add interface.");
        }

        void installFolderInterface();
        void installFileInterface();

    public:
        auto begin() const { return mChildren.begin(); }
        auto end() const { return mChildren.end(); }

        OsIdentifyInfo info() const { return mIdentifyInfo; }

        virtual bool isA(sm::uuid guid) const {
            return mInterfaces.contains(guid);
        }

        /// @brief Read a range of bytes from the file.
        ///
        /// @details This function is expected to be internally synchronized.
        ///
        /// @param request The request to read from the file.
        /// @param result The result of the read operation.
        ///
        /// @return The status of the read operation.
        virtual OsStatus read(ReadRequest, ReadResult*) { return OsStatusNotSupported; }

        /// @brief Write a range of bytes to the file.
        ///
        /// @details This function is expected to be internally synchronized.
        ///
        /// @param request The request to write to the file.
        /// @param result The result of the write operation.
        ///
        /// @return The status of the write operation.
        virtual OsStatus write(WriteRequest, WriteResult*) { return OsStatusNotSupported; }

        /// @brief Create a new file.
        ///
        /// @details This function is expected to be internally synchronized.
        ///
        /// @param node The new file node.
        ///
        /// @return The status of the create operation.
        virtual OsStatus create(IVfsNode**) { return OsStatusNotSupported; }

        /// @brief Create a new directory.
        ///
        /// @details This function is expected to be internally synchronized.
        ///
        /// @param node The new directory node.
        ///
        /// @return The status of the create operation.
        virtual OsStatus mkdir(IVfsNode**) { return OsStatusNotSupported; }

        /// @brief Remove a file.
        ///
        /// @details This function is expected to be internally synchronized.
        ///
        /// @param node The file to remove.
        ///
        /// @return The status of the remove operation.
        virtual OsStatus remove(IVfsNode*) { return OsStatusNotSupported; }

        /// @brief Remove a directory.
        ///
        /// @details This function is expected to be internally synchronized.
        ///
        /// @param node The directory to remove.
        ///
        /// @return The status of the remove operation.
        virtual OsStatus rmdir(IVfsNode*) { return OsStatusNotSupported; }

        /// @brief Stat the file.
        ///
        /// @details This function is expected to be internally synchronized.
        ///
        /// @param stat The stat structure to fill.
        ///
        /// @return The status of the stat operation.
        virtual OsStatus stat(VfsNodeStat*) { return OsStatusNotSupported; }

        /// @brief Open this node as a device.
        ///
        /// @details This function is expected to be internally synchronized.
        ///
        /// @param uuid The UUID of the interface being requested.
        /// @param handle The handle to the device.
        ///
        /// @return The status of the open operation.
        virtual OsStatus query(sm::uuid uuid, IVfsNodeHandle **handle);

        virtual OsStatus open(IVfsNodeHandle **handle);

        virtual OsStatus opendir(IVfsNodeHandle **handle);

        virtual OsStatus lookup(VfsStringView name, IVfsNode **child);
        OsStatus addFile(VfsStringView name, IVfsNode **child);
        OsStatus addFolder(VfsStringView name, IVfsNode **child);
        OsStatus addNode(VfsStringView name, IVfsNode *node);

        void initNode(IVfsNode *node, VfsStringView name, VfsNodeType type);

        bool readyForRemove() const { return true; }
    };

    struct IVfsMount {
        const IVfsDriver *driver;

        constexpr IVfsMount(const IVfsDriver *driver)
            : driver(driver)
        { }

        virtual ~IVfsMount() = default;

        virtual OsStatus root(IVfsNode**) { return OsStatusNotSupported; }
    };

    struct IVfsDriver {
        stdx::StringView name;

        constexpr IVfsDriver(stdx::StringView name)
            : name(name)
        { }

        virtual ~IVfsDriver() = default;

        virtual OsStatus mount(IVfsMount**) { return OsStatusNotSupported; }
        virtual OsStatus unmount(IVfsMount*) { return OsStatusNotSupported; }
    };
}
