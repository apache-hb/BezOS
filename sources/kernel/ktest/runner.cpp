#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>

#include <argparse/argparse.hpp>

#include <filesystem>
#include <string>

#include <unistd.h>

using namespace std::string_literals;

namespace fs = std::filesystem;

static std::string VirtLastErrorString() {
    const char *msg = virGetLastErrorMessage();
    if (msg == nullptr) {
        return "No error";
    }

    return msg;
}

static std::string GetTestDomain(std::string id) {
    return "BezOS-IntegrationTest-"s + id;
}

static constexpr const char *kLimineConf = R"(
# Timeout in seconds that Limine will use before automatically booting.
timeout: 0

# The entry name that will be displayed in the boot menu.
/TestImage
    # We use the Limine boot protocol.
    protocol: limine

    # Path to the kernel to boot. boot():/ represents the partition on which limine.conf is located.
    kernel_path: boot():/boot/$0
)";

static constexpr const char *kDomainXml = R"(
<domain type='qemu'>
    <name>$0</name>
    <memory unit='MiB'>128</memory>
    <vcpu placement='static'>4</vcpu>
    <os>
        <type arch='x86_64' machine='pc'>hvm</type>
        <boot dev='cdrom'/>
    </os>
    <features>
        <acpi/>
    </features>
    <on_poweroff>destroy</on_poweroff>
    <on_reboot>restart</on_reboot>
    <on_crash>preserve</on_crash>
    <devices>
        <disk type='file' device='cdrom'>
            <driver name='qemu' type='raw'/>
            <source file='$1'/>
            <target dev='hda' bus='ide'/>
            <readonly/>
            <address type='drive' controller='0' bus='0' target='0' unit='0'/>
        </disk>

        <serial type='file'>
            <source path='/tmp/$0-serial.log'/>
            <target port='0'/>
        </serial>
        <console type='file'>
            <source path='/tmp/$0-serial.log'/>
            <target type='serial' port='0'/>
        </console>
    </devices>
</domain>
)";

int main(int argc, const char **argv) {
    argparse::ArgumentParser program("ktest-runner");
    program.add_argument("--kernel")
        .help("Path to the kernel binary to test")
        .required();

    program.add_argument("--limine-install")
        .help("Path to the Limine install binary")
        .required();

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    std::string kernelPath = program.get<std::string>("--kernel");
    std::string limineInstallPath = program.get<std::string>("--limine-install");

    auto tmpfolder = fs::temp_directory_path() / ("ktest." + std::to_string(getpid()));
    fs::create_directories(tmpfolder);
    fs::create_directories(tmpfolder / "limine" / "image" / "boot");
    fs::create_directories(tmpfolder / "limine" / "image" / "EFI" / "BOOT");

    return 0;
}
