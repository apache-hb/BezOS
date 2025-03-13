#pragma once

#include "fs2/device.hpp"
#include "fs2/folder.hpp"
#include "fs2/identify.hpp"

#include "fs2/node.hpp"

namespace km {
    class SystemObjects;
}

namespace dev {
    static constexpr inline OsIdentifyInfo kRunFsInfo {
        .DisplayName = "Process Management File System",
        .Model = "RunFS",
        .DeviceVendor = "Bezos",
        .DriverVendor = "Bezos",
        .DriverVersion = OS_VERSION(1, 0, 0),
    };

    class RunFsProcess : public vfs2::INode {
        km::SystemObjects *mSystem;
        OsProcessHandle mProcess;

    public:
        RunFsProcess(km::SystemObjects *system, OsProcessHandle process)
            : mSystem(system)
            , mProcess(process)
        { }

        OsStatus query(sm::uuid uuid, const void *, size_t, vfs2::IHandle **handle) override;
    };

    class RunFsProcessList
        : public vfs2::INode
        , public vfs2::ConstIdentifyMixin<kRunFsInfo>
    {
        km::SystemObjects *mSystem;

    public:
        RunFsProcessList(km::SystemObjects *system)
            : mSystem(system)
        { }

        OsStatus query(sm::uuid uuid, const void *, size_t, vfs2::IHandle **handle) override;
    };

    class RunFsRoot
        : public vfs2::BasicNode
        , public vfs2::FolderMixin
        , public vfs2::ConstIdentifyMixin<kRunFsInfo>
    {
        km::SystemObjects *mSystem;

    public:
        RunFsRoot(km::SystemObjects *system)
            : mSystem(system)
        { }

        OsStatus query(sm::uuid uuid, const void *, size_t, vfs2::IHandle **handle) override;
    };

    class RunFsMount : public vfs2::IVfsMount {
        km::SystemObjects *mSystem;
        RunFsRoot *mRoot;

    public:
        RunFsMount(vfs2::IVfsDriver *driver, km::SystemObjects *system)
            : IVfsMount(driver)
            , mSystem(system)
            , mRoot(new RunFsRoot(mSystem))
        { }

        OsStatus root(vfs2::INode **node) override {
            *node = mRoot;
            return OsStatusSuccess;
        }
    };

    class RunFsDriver : public vfs2::IVfsDriver {
        km::SystemObjects *mSystem;

    public:
        OsStatus mount(vfs2::IVfsMount **mount) override;
        OsStatus unmount(vfs2::IVfsMount *mount) override;
    };
}
