#include <filesystem>
#include <gtest/gtest.h>

#include "util/defer.hpp"

#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>

using namespace std::literals;

static void ReplaceAll(std::string &str, const std::string &from, const std::string &to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

static std::string VirtLastErrorString() {
    virErrorPtr error = virGetLastError();
    if (error == nullptr) {
        return "No error";
    }

    std::string message = error->message;
    virFreeError(error);

    return message;
}

static constexpr const char *kDomainXml = R"(
<domain type='qemu'>
    <name>BezOS-IntegrationTest-Launch</name>
    <uuid>9a19e02c-ea6d-11ef-8ed3-00155dd28020</uuid>
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
            <source file='/tmp/bezos-limine.iso'/>
            <target dev='hda' bus='ide'/>
            <readonly/>
            <address type='drive' controller='0' bus='0' target='0' unit='0'/>
        </disk>
    </devices>
</domain>
)";

static std::string ShellExec(std::string cmd) {
    int status;
    std::string result;
    FILE *pipe = popen(cmd.c_str(), "r");
    if (pipe == nullptr) {
        throw std::runtime_error("popen() failed");
    }

    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }

    status = pclose(pipe);
    if (status == -1) {
        throw std::runtime_error("pclose() failed");
    }

    return result;
}

static std::string GetCwd() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == nullptr) {
        throw std::runtime_error("getcwd() failed");
    }

    return std::string(cwd);
}

static std::string GetEnvElement(const char *name) {
    const char *value = getenv(name);
    if (value == nullptr) {
        throw std::runtime_error("getenv() failed "s + std::to_string(errno));
    }

    return std::string(value);
}

//
// Create a domain using a kernel image, launch it, and then destroy it.
//
TEST(LaunchTest, Launch) {
    ASSERT_TRUE(true);

    virConnectPtr conn = virConnectOpen("qemu:///system");
    ASSERT_NE(conn, nullptr) << VirtLastErrorString();
    defer { virConnectClose(conn); };

    char *uri = virConnectGetURI(conn);
    ASSERT_NE(uri, nullptr) << VirtLastErrorString();
    defer { free(uri); };

    const char *hvType = virConnectGetType(conn);
    ASSERT_NE(hvType, nullptr) << VirtLastErrorString();

    auto qemuSystemX86_64 = ShellExec("which qemu-system-x86_64");
    ASSERT_NE(qemuSystemX86_64, "") << "qemu-system-x86_64 not found";

    auto iso = GetEnvElement("MESON_PREFIX") + "/bezos-limine.iso";
    std::filesystem::copy_file(iso, "/tmp/bezos-limine.iso", std::filesystem::copy_options::overwrite_existing);

    virDomainPtr domain = virDomainDefineXMLFlags(conn, kDomainXml, VIR_DOMAIN_DEFINE_VALIDATE);
    ASSERT_NE(domain, nullptr) << VirtLastErrorString() << kDomainXml;
    defer { virDomainFree(domain); };

    int ret = virDomainCreate(domain);
    ASSERT_EQ(ret, 0) << VirtLastErrorString();

    virDomainInfo info;
    ret = virDomainGetInfo(domain, &info);
    ASSERT_EQ(ret, 0) << VirtLastErrorString();

    int state;
    ret = virDomainGetState(domain, &state, nullptr, 0);
    ASSERT_EQ(ret, 0) << VirtLastErrorString();

    ASSERT_EQ(state, VIR_DOMAIN_RUNNING) << state;

    ret = virDomainDestroy(domain);
    ASSERT_EQ(ret, 0) << VirtLastErrorString();

    ret = virDomainUndefine(domain);
    ASSERT_EQ(ret, 0) << VirtLastErrorString();
}
