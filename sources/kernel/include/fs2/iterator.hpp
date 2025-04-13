#pragma once

#include <bezos/status.h>

#include "fs2/folder.hpp"

namespace vfs2 {
    template<std::derived_from<FolderMixin> T>
    class TIteratorHandle : public BaseHandle<T, IIteratorHandle> {
        using BaseHandle<T, IIteratorHandle>::mNode;
        using Iterator = FolderMixin::Iterator;

        Iterator mCurrent{};

    public:
        TIteratorHandle(sm::RcuSharedPtr<T> node, const void *, size_t)
            : BaseHandle<T, IIteratorHandle>(node)
        { }

        OsStatus next(sm::RcuSharedPtr<INode> *node) override {
            sm::RcuSharedPtr<INode> result = nullptr;
            if (OsStatus status = mNode->next(&mCurrent, &result)) {
                return status;
            }

            *node = result;
            return OsStatusSuccess;
        }

        HandleInfo info() override {
            return HandleInfo { mNode, kOsIteratorGuid };
        }
    };
}
