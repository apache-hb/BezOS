#include "pkgtoold/proc_mounts.hpp"

#include <gtest/gtest.h>

class ProcMountsTest : public testing::Test {
};

// Pulled from my dev machine running WSL2
static constexpr char kWslExample[] = R"(
none /mnt/wsl tmpfs rw,relatime 0 0
drivers /usr/lib/wsl/drivers 9p ro,nosuid,nodev,noatime,aname=drivers;fmask=222;dmask=222,cache=5,access=client,msize=65536,trans=fd,rfd=8,wfd=8 0 0
/dev/sdc / ext4 rw,relatime,discard,errors=remount-ro,data=ordered 0 0
none /mnt/wslg tmpfs rw,relatime 0 0
/dev/sdc /mnt/wslg/distro ext4 ro,relatime,discard,errors=remount-ro,data=ordered 0 0
none /usr/lib/wsl/lib overlay rw,nosuid,nodev,noatime,lowerdir=/gpu_lib_packaged:/gpu_lib_inbox,upperdir=/gpu_lib/rw/upper,workdir=/gpu_lib/rw/work,uuid=on 0 0
rootfs /init rootfs ro,size=98828576k,nr_inodes=24707144 0 0
none /dev devtmpfs rw,nosuid,relatime,size=98828580k,nr_inodes=24707145,mode=755 0 0
sysfs /sys sysfs rw,nosuid,nodev,noexec,noatime 0 0
proc /proc proc rw,nosuid,nodev,noexec,noatime 0 0
devpts /dev/pts devpts rw,nosuid,noexec,noatime,gid=5,mode=620,ptmxmode=000 0 0
none /run tmpfs rw,nosuid,nodev,mode=755 0 0
none /run/lock tmpfs rw,nosuid,nodev,noexec,noatime 0 0
none /run/shm tmpfs rw,nosuid,nodev,noatime 0 0
none /dev/shm tmpfs rw,nosuid,nodev,noatime 0 0
none /run/user tmpfs rw,nosuid,nodev,noexec,noatime,mode=755 0 0
binfmt_misc /proc/sys/fs/binfmt_misc binfmt_misc rw,relatime 0 0
cgroup2 /sys/fs/cgroup cgroup2 rw,nosuid,nodev,noexec,relatime,nsdelegate 0 0
none /mnt/wslg/versions.txt overlay rw,relatime,lowerdir=/systemvhd,upperdir=/system/rw/upper,workdir=/system/rw/work,uuid=on 0 0
none /mnt/wslg/doc overlay rw,relatime,lowerdir=/systemvhd,upperdir=/system/rw/upper,workdir=/system/rw/work,uuid=on 0 0
none /tmp/.X11-unix tmpfs ro,relatime 0 0
C:\134 /mnt/c 9p rw,noatime,aname=drvfs;path=C:\;uid=1000;gid=1000;symlinkroot=/mnt/,cache=5,access=client,msize=65536,trans=fd,rfd=6,wfd=6 0 0
D:\134 /mnt/d 9p rw,noatime,aname=drvfs;path=D:\;uid=1000;gid=1000;symlinkroot=/mnt/,cache=5,access=client,msize=65536,trans=fd,rfd=6,wfd=6 0 0
E:\134 /mnt/e 9p rw,noatime,aname=drvfs;path=E:\;uid=1000;gid=1000;symlinkroot=/mnt/,cache=5,access=client,msize=65536,trans=fd,rfd=6,wfd=6 0 0
F:\134 /mnt/f 9p rw,noatime,aname=drvfs;path=F:\;uid=1000;gid=1000;symlinkroot=/mnt/,cache=5,access=client,msize=65536,trans=fd,rfd=6,wfd=6 0 0
none /run/user tmpfs rw,relatime 0 0
hugetlbfs /dev/hugepages hugetlbfs rw,relatime,pagesize=2M 0 0
mqueue /dev/mqueue mqueue rw,nosuid,nodev,noexec,relatime 0 0
debugfs /sys/kernel/debug debugfs rw,nosuid,nodev,noexec,relatime 0 0
tracefs /sys/kernel/tracing tracefs rw,nosuid,nodev,noexec,relatime 0 0
fusectl /sys/fs/fuse/connections fusectl rw,nosuid,nodev,noexec,relatime 0 0
tmpfs /run/qemu tmpfs rw,nosuid,nodev,relatime,mode=755 0 0
overlay /home/elliothb/github/bezos/build/env/image/target/sysroot overlay rw,relatime,lowerdir=/home/elliothb/github/bezos/build/env/initrd/target/install,upperdir=/home/elliothb/github/bezos/build/env/image/target/install,workdir=/home/elliothb/github/bezos/build/env/image/target/internal/work 0 0
/dev/sdc /var/lib/docker ext4 rw,relatime,discard,errors=remount-ro,data=ordered 0 0
tmpfs /run/user/1000 tmpfs rw,nosuid,nodev,relatime,size=19766860k,nr_inodes=4941715,mode=700,uid=1000,gid=1000 0 0
tmpfs /mnt/wslg/run/user/1000 tmpfs rw,nosuid,nodev,relatime,size=19766860k,nr_inodes=4941715,mode=700,uid=1000,gid=1000 0 0
overlay /home/elliothb/github/bezos/build/tool/test_environments/WorkspaceTest_BuildDependantPackage/workspace/build/env/package002/target/sysroot overlay rw,relatime,lowerdir=/home/elliothb/github/bezos/build/tool/test_environments/WorkspaceTest_BuildDependantPackage/workspace/build/env/package001/target/install:/home/elliothb/github/bezos/build/tool/test_environments/WorkspaceTest_BuildDependantPackage/workspace/build/env/package003/target/install,upperdir=/home/elliothb/github/bezos/build/tool/test_environments/WorkspaceTest_BuildDependantPackage/workspace/build/env/package002/target/install,workdir=/home/elliothb/github/bezos/build/tool/test_environments/WorkspaceTest_BuildDependantPackage/workspace/build/env/package002/target/internal/work,uuid=on 0 0
)";

TEST_F(ProcMountsTest, ParseProcMounts) {
    std::istringstream is{kWslExample};
    pkg::ProcMounts mounts{is};

    auto entries = mounts.entries();
    ASSERT_EQ(entries.size(), 37);

    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        ASSERT_FALSE(entry.device.empty()) << "Entry " << i << " with empty device found";
        ASSERT_FALSE(entry.mount.empty()) << "Entry " << i << " with empty mount found";
        ASSERT_FALSE(entry.fstype.empty()) << "Entry " << i << " with empty fstype found";
        ASSERT_FALSE(entry.options.empty()) << "Entry " << i << " with empty options found";
    }

    auto overlay = std::find_if(entries.begin(), entries.end(),
        [](const auto& entry) {
            return entry.device == "overlay" && entry.mount == "/home/elliothb/github/bezos/build/tool/test_environments/WorkspaceTest_BuildDependantPackage/workspace/build/env/package002/target/sysroot";
        }
    );

    ASSERT_NE(overlay, entries.end());
    ASSERT_EQ(overlay->fstype, "overlay");
    ASSERT_EQ(overlay->options, "rw,relatime,lowerdir=/home/elliothb/github/bezos/build/tool/test_environments/WorkspaceTest_BuildDependantPackage/workspace/build/env/package001/target/install:/home/elliothb/github/bezos/build/tool/test_environments/WorkspaceTest_BuildDependantPackage/workspace/build/env/package003/target/install,upperdir=/home/elliothb/github/bezos/build/tool/test_environments/WorkspaceTest_BuildDependantPackage/workspace/build/env/package002/target/install,workdir=/home/elliothb/github/bezos/build/tool/test_environments/WorkspaceTest_BuildDependantPackage/workspace/build/env/package002/target/internal/work,uuid=on");
}

TEST_F(ProcMountsTest, OfCurrentMachine) {
    auto mounts = pkg::ProcMounts::ofCurrentMachine();
    auto entries = mounts.entries();

    ASSERT_FALSE(entries.empty()) << "No mount entries found on current machine";

    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        ASSERT_FALSE(entry.device.empty()) << "Entry " << i << " with empty device found";
        ASSERT_FALSE(entry.mount.empty()) << "Entry " << i << " with empty mount found";
        ASSERT_FALSE(entry.fstype.empty()) << "Entry " << i << " with empty fstype found";
        ASSERT_FALSE(entry.options.empty()) << "Entry " << i << " with empty options found";
    }
}

TEST_F(ProcMountsTest, ParseOverlayEntries) {
    std::istringstream is{kWslExample};
    pkg::ProcMounts mounts{is};

    auto overlays = mounts.overlayEntries();
    ASSERT_EQ(overlays.size(), 5);

    const auto& firstOverlay = overlays[0];
    ASSERT_EQ(firstOverlay.overlay, "/usr/lib/wsl/lib");
    ASSERT_EQ(firstOverlay.upper, "/gpu_lib/rw/upper");
    ASSERT_EQ(firstOverlay.work, "/gpu_lib/rw/work");
    ASSERT_EQ(firstOverlay.lowers.size(), 2);
    ASSERT_EQ(firstOverlay.lowers[0], "/gpu_lib_packaged");
    ASSERT_EQ(firstOverlay.lowers[1], "/gpu_lib_inbox");

    const auto& last = overlays.back();
    ASSERT_EQ(last.overlay, "/home/elliothb/github/bezos/build/tool/test_environments/WorkspaceTest_BuildDependantPackage/workspace/build/env/package002/target/sysroot");
    ASSERT_EQ(last.upper, "/home/elliothb/github/bezos/build/tool/test_environments/WorkspaceTest_BuildDependantPackage/workspace/build/env/package002/target/install");
    ASSERT_EQ(last.work, "/home/elliothb/github/bezos/build/tool/test_environments/WorkspaceTest_BuildDependantPackage/workspace/build/env/package002/target/internal/work");
    ASSERT_EQ(last.lowers.size(), 2);
    ASSERT_EQ(last.lowers[0], "/home/elliothb/github/bezos/build/tool/test_environments/WorkspaceTest_BuildDependantPackage/workspace/build/env/package001/target/install");
    ASSERT_EQ(last.lowers[1], "/home/elliothb/github/bezos/build/tool/test_environments/WorkspaceTest_BuildDependantPackage/workspace/build/env/package003/target/install");
}
