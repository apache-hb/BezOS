#pragma once

#include "fs/device.hpp"
#include "fs/folder.hpp"
#include "fs/identify.hpp"

#include "fs/node.hpp"

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

    class RunFsProcess
        : public vfs::INode
        , public vfs::FolderMixin
    {
        km::SystemObjects *mSystem;
        OsProcessHandle mProcess;

    public:
        RunFsProcess(km::SystemObjects *system, OsProcessHandle process);

        OsStatus query(sm::uuid uuid, const void *, size_t, vfs::IHandle **handle) override;
    };

    class RunFsProcessList
        : public vfs::BasicNode
        , public vfs::ConstIdentifyMixin<kRunFsInfo>
    {
        km::SystemObjects *mSystem;

    public:
        RunFsProcessList(km::SystemObjects *system);

        OsStatus query(sm::uuid uuid, const void *, size_t, vfs::IHandle **handle) override;
    };

    class RunFsRoot
        : public vfs::BasicNode
        , public vfs::FolderMixin
        , public vfs::ConstIdentifyMixin<kRunFsInfo>
    {
        km::SystemObjects *mSystem;

    public:
        RunFsRoot(km::SystemObjects *system);

        OsStatus query(sm::uuid uuid, const void *, size_t, vfs::IHandle **handle) override;
    };

    class RunFsMount : public vfs::IVfsMount {
        km::SystemObjects *mSystem;
        RunFsRoot *mRoot;

    public:
        RunFsMount(vfs::IVfsDriver *driver, km::SystemObjects *system)
            : IVfsMount(driver)
            , mSystem(system)
            , mRoot(new RunFsRoot(mSystem))
        { }

        OsStatus root(vfs::INode **node) override {
            *node = mRoot;
            return OsStatusSuccess;
        }
    };

    class RunFsDriver : public vfs::IVfsDriver {
        km::SystemObjects *mSystem;

    public:
        OsStatus mount(vfs::IVfsMount **mount) override;
        OsStatus unmount(vfs::IVfsMount *mount) override;
    };
}
