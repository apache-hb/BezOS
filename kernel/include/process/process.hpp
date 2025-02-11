#pragma once

#include "fs2/node.hpp"
#include "isr.hpp"
#include "std/string.hpp"

namespace km {
    using ThreadId = uint64_t;

    class Thread {

    };

    class Process {
        stdx::String mName;
        Privilege mPrivilege;
        stdx::Vector<ThreadId> mThreads;
        stdx::Vector2<std::unique_ptr<vfs2::IVfsNodeHandle>> mFiles;

    public:
        Process();
    };
}
