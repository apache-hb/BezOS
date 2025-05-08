#pragma once

#include "fs/base.hpp"
#include "fs/folder.hpp"
#include "fs/identify.hpp"
#include "pci/pci.hpp"

namespace pci {
    class IConfigSpace;
}

namespace dev {
    class PciRoot;
    class PciBus;
    class PciSlot;
    class PciFunction;
    class PciCapability;

    static constexpr inline OsIdentifyInfo kPciRootInfo {
        .DisplayName = "PCI Root",
        .Model = "PCI",
        .DeviceVendor = "BezOS",
        .FirmwareRevision = "1.0.0",
        .DriverVendor = "BezOS",
        .DriverVersion = OS_VERSION(1, 0, 0),
    };

    class PciCapability final : public vfs::BasicNode {
        pci::IConfigSpace *mConfigSpace;
        pci::Capability mCapability;
    public:
        OsStatus query(sm::uuid uuid, const void *data, size_t size, vfs::IHandle **handle) override;
    };

    class PciFunction final : public vfs::BasicNode, public vfs::FolderMixin {
        pci::IConfigSpace *mConfigSpace;
        pci::ConfigHeader mHeader;
    public:
        OsStatus query(sm::uuid uuid, const void *data, size_t size, vfs::IHandle **handle) override;
    };

    class PciSlot final : public vfs::BasicNode, public vfs::FolderMixin {
        pci::IConfigSpace *mConfigSpace;
        uint8_t mBus;
        uint8_t mSlot;
    public:
        OsStatus query(sm::uuid uuid, const void *data, size_t size, vfs::IHandle **handle) override;
    };

    class PciBus final : public vfs::BasicNode, public vfs::FolderMixin {
        pci::IConfigSpace *mConfigSpace;
        uint8_t mBus;
    public:
        OsStatus query(sm::uuid uuid, const void *data, size_t size, vfs::IHandle **handle) override;
    };

    class PciRoot final : public vfs::BasicNode, public vfs::FolderMixin, public vfs::ConstIdentifyMixin<kPciRootInfo> {
        pci::IConfigSpace *mConfigSpace;
    public:
        OsStatus query(sm::uuid uuid, const void *data, size_t size, vfs::IHandle **handle) override;
    };
}
