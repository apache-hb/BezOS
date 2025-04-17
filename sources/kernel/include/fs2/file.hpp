#pragma once

#include "fs2/base.hpp"
#include "fs2/device.hpp"
#include "fs2/interface.hpp"

namespace vfs2 {
    template<typename T>
    concept FileNodeStat = requires (T it) {
        { it.stat(std::declval<OsFileInfo*>()) } -> std::same_as<OsStatus>;
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
    class TFileHandle : public BaseHandle<T, IFileHandle> {
        using BaseHandle<T, IFileHandle>::mNode;
    public:
        TFileHandle(sm::RcuSharedPtr<T> node, const void *, size_t)
            : BaseHandle<T, IFileHandle>(node)
        { }

        virtual HandleInfo info() override {
            return HandleInfo { mNode, kOsFileGuid };
        }


        virtual OsStatus stat(OsFileInfo *info) override {
            return mNode->stat(info);
        }

        OsStatus stat(void *data, size_t size) {
            if ((size != sizeof(OsFileInfo)) || (data == nullptr)) {
                return OsStatusInvalidData;
            }

            return stat(static_cast<OsFileInfo*>(data));
        }

        virtual OsStatus invoke(IInvokeContext *, uint64_t method, void *data, size_t size) override {
            switch (method) {
            case eOsFileStat:
                return stat(data, size);
            default:
                return OsStatusFunctionNotSupported;
            }
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
