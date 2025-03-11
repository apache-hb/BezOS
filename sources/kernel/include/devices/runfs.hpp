#pragma once

#include "fs2/node.hpp"

namespace km {
    class SystemObjects;
}

namespace dev {
    class RunFsProcess : public vfs2::IVfsNode {

    };

    class RunFsRoot : public vfs2::IVfsNode {
        km::SystemObjects *mSystem;

    public:
    };

    class RunFsMount : public vfs2::IVfsMount {
        km::SystemObjects *mSystem;
        RunFsRoot *mRoot;

    public:
        OsStatus root(vfs2::IVfsNode **node) override;
    };

    class RunFsDriver : public vfs2::IVfsDriver {
        km::SystemObjects *mSystem;

    public:
        OsStatus mount(vfs2::IVfsMount **mount) override;
        OsStatus unmount(vfs2::IVfsMount *mount) override;
    };
}
