#pragma once

#include <bezos/status.h>

#include "fs2/folder.hpp"

namespace vfs2 {
    template<std::derived_from<FolderMixin> T>
    class TIteratorHandle : public BasicHandle<T, IIteratorHandle> {
        using BasicHandle<T, IIteratorHandle>::mNode;
        using Iterator = FolderMixin::Iterator;

        Iterator mCurrent{};

    public:
        TIteratorHandle(T *node, const void *, size_t)
            : BasicHandle<T, IIteratorHandle>(node)
        { }

        OsStatus next(INode **node) override {
            INode *result = nullptr;
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
