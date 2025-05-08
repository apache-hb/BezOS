#pragma once

#include "fs/base.hpp"
#include "fs/device.hpp"

namespace vfs {
    template<typename T>
    concept StreamNodeRead = requires (T it) {
        { it.read(std::declval<ReadRequest>(), std::declval<ReadResult*>()) } -> std::same_as<OsStatus>;
    };

    template<typename T>
    concept StreamNodeWrite = requires (T it) {
        { it.write(std::declval<WriteRequest>(), std::declval<WriteResult*>()) } -> std::same_as<OsStatus>;
    };

    template<typename T>
    concept StreamNode = std::derived_from<T, INode> && StreamNodeRead<T> && StreamNodeWrite<T>;

    template<StreamNode T>
    class TStreamHandle : public BaseHandle<T, IHandle> {
        using BaseHandle<T, IHandle>::mNode;
    public:
        TStreamHandle(sm::RcuSharedPtr<T> node, const void *, size_t)
            : BaseHandle<T, IHandle>(node)
        { }

        virtual OsStatus read(ReadRequest request, ReadResult *result) override {
            return mNode->read(request, result);
        }

        virtual OsStatus write(WriteRequest request, WriteResult *result) override {
            return mNode->write(request, result);
        }

        HandleInfo info() override {
            return HandleInfo { mNode, kOsStreamGuid };
        }
    };
}
