#pragma once

#include "fs2/device.hpp"
#include "fs2/interface.hpp"

namespace vfs2 {
    template<typename T>
    concept FileNodeStat = requires (T it) {
        { it.stat(std::declval<NodeStat*>()) } -> std::same_as<OsStatus>;
    };

    template<typename T>
    concept FileNodeRead = requires (T it) {
        { it.read(std::declval<ReadRequest>(), std::declval<ReadResult*>()) } -> std::same_as<OsStatus>;
    };

    template<typename T>
    concept FileNodeWrite = requires (T it) {
        { it.write(std::declval<WriteRequest>(), std::declval<WriteResult*>()) } -> std::same_as<OsStatus>;
    };

    template<typename T>
    concept FileNode = FileNodeStat<T> && std::derived_from<T, INode>;

    template<FileNode T>
    class TFileHandle : public BasicHandle<T, IFileHandle> {
        using BasicHandle<T, IFileHandle>::mNode;
    public:
        TFileHandle(sm::RcuSharedPtr<T> node, const void *, size_t)
            : BasicHandle<T, IFileHandle>(node)
        { }

        virtual HandleInfo info() override {
            return HandleInfo { mNode, kOsFileGuid };
        }

        virtual OsStatus stat(NodeStat *stat) override {
            return mNode->stat(stat);
        }

        virtual OsStatus read(ReadRequest request, ReadResult *result) override {
            if constexpr (FileNodeRead<T>) {
                return mNode->read(request, result);
            } else {
                return OsStatusNotSupported;
            }
        }

        virtual OsStatus write(WriteRequest request, WriteResult *result) override {
            if constexpr (FileNodeWrite<T>) {
                return mNode->write(request, result);
            } else {
                return OsStatusNotSupported;
            }
        }
    };
}
