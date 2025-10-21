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

static std::string getEnvElement(const char *name) {
    const char *value = getenv(name);
    if (value == nullptr) {
        throw std::runtime_error("getenv() failed "s + std::to_string(errno));
    }

    return std::string(value);
}

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

int main(int argc, const char **argv) try {
    argparse::ArgumentParser program("ktest-runner");
    program.add_argument("--kernel")
        .help("Path to the kernel binary to test")
        .required();

    program.add_argument("--limine-install")
        .help("Path to the Limine install binary")
        .required();

    program.add_argument("--test-program")
        .help("Path to the test program to run in the kernel");

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
    auto initrdFolder = tmpfolder / "initrd";
    auto buildFolder = tmpfolder / "build";
    auto runFolder = tmpfolder / "run";
    auto imageFolder = buildFolder / "limine" / "image";
    auto bootPath = imageFolder / "boot";
    auto limineBootPath = bootPath / "limine";
    auto limineEfiPath = imageFolder / "EFI" / "BOOT";
    auto kernelIso = runFolder / "bezos-limine.iso";

    fs::create_directories(initrdFolder);
    fs::create_directories(buildFolder);
    fs::create_directories(runFolder);
    fs::create_directories(limineBootPath);
    fs::create_directories(limineEfiPath);

    if (program.is_used("--test-program")) {
        std::string testProgramPath = program.get<std::string>("--test-program");
        fs::copy_file(testProgramPath, initrdFolder / "ktest-init", fs::copy_options::overwrite_existing);

        int tar = subprocess::call({
            "tar", "-C", initrdFolder.string().c_str(), "-cf",
            (buildFolder / "initrd.tar").string().c_str(),
            "."
        });

        if (tar != 0) {
            std::cerr << "Failed to create initrd tarball" << std::endl;
            return 1;
        }

        fs::copy_file(buildFolder / "initrd.tar", bootPath / "initrd" / "initrd.tar", fs::copy_options::overwrite_existing);
    }

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

    return 0;
} catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
} catch (...) {
    std::cerr << "Unknown error occurred." << std::endl;
    return 1;
}
