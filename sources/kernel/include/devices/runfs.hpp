#pragma once

#include "fs2/node.hpp"

namespace km {
    class SystemObjects;
}

namespace dev {
    class RunFsProcess : public vfs2::IVfsNode {

    };

    class RunFsMount : public vfs2::IVfsMount {

    };

    class RunFsDriver : public vfs2::IVfsDriver {
        km::SystemObjects *mSystem;

    public:
    };
}
