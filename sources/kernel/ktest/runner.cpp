#if 0
#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#endif

#include <argparse/argparse.hpp>
#include <cpp-subprocess/subprocess.hpp>

#include <filesystem>
#include <string>
#include <fstream>

#include <unistd.h>

using namespace std::string_literals;

namespace fs = std::filesystem;

#define KT_ASSERT(condition, message) \
    if (!(condition)) { \
        throw std::runtime_error("Assertion failed: "s + message); \
    }

#define KT_ASSERT_EQ(condition, value, message) \
    if ((condition) != (value)) { \
        throw std::runtime_error("Assertion failed: "s + message + ", expected " + std::to_string(value) + ", got " + std::to_string(condition)); \
    }

#define KT_ASSERT_NE(condition, value, message) \
    if ((condition) == (value)) { \
        throw std::runtime_error("Assertion failed: "s + message); \
    }

#if 0
static void replaceAll(std::string &str, const std::string &from, const std::string &to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

static std::string virtLastErrorString() {
    const char *msg = virGetLastErrorMessage();
    if (msg == nullptr) {
        return "No error";
    }

    return msg;
}

static std::string getTestDomain(std::string id) {
    return "BezOS-IntegrationTest-"s + id;
}
#endif

static std::string getEnvElement(const char *name) {
    const char *value = getenv(name);
    if (value == nullptr) {
        throw std::runtime_error("getenv() failed "s + std::to_string(errno));
    }

    return std::string(value);
}

#if 0
static void assertDomainState(virDomainPtr ptr, int expected) {
    int state;
    int ret = virDomainGetState(ptr, &state, nullptr, 0);
    if (ret < 0) {
        throw std::runtime_error(std::format("Failed to get domain state: {}", virtLastErrorString()));
    }
    if (state != expected) {
        throw std::runtime_error(std::format("Domain state mismatch: expected {}, got {}", expected, state));
    }
}

static void killDomain(virConnectPtr conn, const std::string &name) {
    virDomainPtr domain = virDomainLookupByName(conn, name.c_str());
    if (domain == nullptr) {
        return;
    }

    int ret = virDomainDestroy(domain);
    if (ret != 0) {
        std::cerr << "Failed to destroy domain: " << virtLastErrorString() << std::endl;
    }

    ret = virDomainUndefine(domain);
    if (ret != 0) {
        std::cerr << "Failed to undefine domain: " << virtLastErrorString() << std::endl;
    }
}
#endif

static constexpr const char *kLimineConf = R"(
# Timeout in seconds that Limine will use before automatically booting.
timeout: 0

# The entry name that will be displayed in the boot menu.
/TestImage
    # We use the Limine boot protocol.
    protocol: limine

    # Path to the kernel to boot. boot():/ represents the partition on which limine.conf is located.
    kernel_path: boot():/boot/kernel.elf
)";

#if 0
static constexpr const char *kDomainXml = R"(
<domain type='qemu'>
    <name>$0</name>
    <memory unit='MiB'>128</memory>
    <vcpu placement='static'>4</vcpu>
    <os>
        <type arch='x86_64' machine='pc'>hvm</type>
        <boot dev='cdrom' />
    </os>
    <features>
        <acpi/>
    </features>
    <on_poweroff>destroy</on_poweroff>
    <on_reboot>restart</on_reboot>
    <on_crash>preserve</on_crash>
    <devices>
        <disk type='file' device='cdrom'>
            <driver name='qemu' type='raw' />
            <source file='$1' />
            <target dev='hda' bus='ide' />
            <readonly />
            <address type='drive' controller='0' bus='0' target='0' unit='0' />
        </disk>

        <serial type='file'>
            <source path='/tmp/$0-serial.log' />
            <target port='0' />
        </serial>
        <console type='file'>
            <source path='/tmp/$0-serial.log' />
            <target type='serial' port='0' />
        </console>
    </devices>
</domain>
)";
#endif

int main(int argc, const char **argv) try {
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

    fs::path globalInstallPrefix = (fs::path(getEnvElement("MESON_PREFIX")) / "..").lexically_normal();
    auto limineSharePath = globalInstallPrefix / "limine" / "share";

    std::string kernelPath = program.get<std::string>("--kernel");
    std::string limineInstallPath = program.get<std::string>("--limine-install");

    auto tmpfolder = fs::temp_directory_path() / ("ktest." + std::to_string(getpid()));
    auto buildFolder = tmpfolder / "build";
    auto runFolder = tmpfolder / "run";
    auto imageFolder = buildFolder / "limine" / "image";
    auto bootPath = imageFolder / "boot";
    auto limineBootPath = bootPath / "limine";
    auto limineEfiPath = imageFolder / "EFI" / "BOOT";
    auto kernelIso = runFolder / "bezos-limine.iso";

    fs::create_directories(buildFolder);
    fs::create_directories(runFolder);
    fs::create_directories(limineBootPath);
    fs::create_directories(limineEfiPath);

    fs::copy_file(kernelPath, bootPath / "kernel.elf", fs::copy_options::overwrite_existing);

    std::fstream limineConfFile((limineBootPath / "limine.conf"), std::ios::out | std::ios::trunc);
    limineConfFile << kLimineConf;
    limineConfFile.close();

    fs::copy_file(limineSharePath / "limine-bios-cd.bin", limineBootPath / "limine-bios-cd.bin", fs::copy_options::overwrite_existing);
    fs::copy_file(limineSharePath / "limine-bios.sys", limineBootPath / "limine-bios.sys", fs::copy_options::overwrite_existing);
    fs::copy_file(limineSharePath / "limine-uefi-cd.bin", limineBootPath / "limine-uefi-cd.bin", fs::copy_options::overwrite_existing);
    fs::copy_file(limineSharePath / "limine-bios-pxe.bin", limineBootPath / "limine-bios-pxe.bin", fs::copy_options::overwrite_existing);
    fs::copy_file(limineSharePath / "BOOTX64.EFI", limineEfiPath / "BOOTX64.EFI", fs::copy_options::overwrite_existing);
    fs::copy_file(limineSharePath / "BOOTIA32.EFI", limineEfiPath / "BOOTIA32.EFI", fs::copy_options::overwrite_existing);

    int xorrsio = subprocess::call({
        "xorriso", "-as", "mkisofs", "-R", "-r", "-J", "-b", "boot/limine/limine-bios-cd.bin",
        "-no-emul-boot", "-boot-load-size", "4", "-boot-info-table", "-hfsplus",
        "-apm-block-size", "2048", "--efi-boot", "boot/limine/limine-uefi-cd.bin",
        "-efi-boot-part", "--efi-boot-image", "--protective-msdos-label",
        imageFolder.string(), "-o", kernelIso.string()
    });

    if (xorrsio != 0) {
        std::cerr << "Failed to create ISO image" << std::endl;
        return 1;
    }

    int install = subprocess::call({
        limineInstallPath, "bios-install", kernelIso.string()
    });

    if (install != 0) {
        std::cerr << "Failed to install Limine" << std::endl;
        return 1;
    }

    auto result = subprocess::call({
        "qemu-system-x86_64",
        "-cdrom", kernelIso.string(),
        "-M", "q35",
        "-display", "none",
        "-chardev", "stdio,id=char0,signal=off,logfile=ktest-runner-qemu.log",
        "-no-reboot",
        "-serial", "chardev:char0",
        "-device", "isa-debug-exit,iobase=0x501,iosize=0x02",
    });

    //
    // qemus isa-debug-exit device exits with (code << 1) | 1
    //
    if (result == 1) {
        return 0;
    }

    std::cerr << "QEMU exited with code: " << result << std::endl;
    return (result >> 1);

#if 0
    virConnectPtr conn = virConnectOpen("qemu:///system");
    KT_ASSERT_NE(conn, nullptr, virtLastErrorString());

    char *uri = virConnectGetURI(conn);
    KT_ASSERT_NE(uri, nullptr, virtLastErrorString());

    const char *hvType = virConnectGetType(conn);
    KT_ASSERT_NE(hvType, nullptr, virtLastErrorString());

    auto name = getTestDomain("Launch");

    // If a domain already exists for this test, kill it.
    killDomain(conn, name);

    std::string xml = kDomainXml;
    replaceAll(xml, "$0", name);
    replaceAll(xml, "$1", kernelIso.string());

    virDomainPtr domain = virDomainDefineXMLFlags(conn, xml.c_str(), VIR_DOMAIN_DEFINE_VALIDATE);
    KT_ASSERT_NE(domain, nullptr, virtLastErrorString() + xml);

    int ret = virDomainCreate(domain);
    KT_ASSERT_EQ(ret, 0, virtLastErrorString());

    virDomainInfo info;
    ret = virDomainGetInfo(domain, &info);
    KT_ASSERT_EQ(ret, 0, virtLastErrorString());

    assertDomainState(domain, VIR_DOMAIN_RUNNING);

    ret = virDomainDestroy(domain);
    KT_ASSERT_EQ(ret, 0, virtLastErrorString());

    virDomainFree(domain);
    free(uri);
    virConnectClose(conn);
#endif

    return 0;
} catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
} catch (...) {
    std::cerr << "Unknown error occurred." << std::endl;
    return 1;
}
