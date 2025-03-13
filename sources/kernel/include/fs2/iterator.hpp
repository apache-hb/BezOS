#pragma once

#include <bezos/status.h>

#include "fs2/folder.hpp"

namespace vfs2 {
    template<std::derived_from<FolderMixin> T>
    class TIteratorHandle : public IIteratorHandle {
        using Iterator = FolderMixin::Iterator;

        T *mNode;
        Iterator mCurrent{};

    public:
        TIteratorHandle(T *node)
            : mNode(node)
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
            return HandleInfo { mNode };
        }
    };
}
