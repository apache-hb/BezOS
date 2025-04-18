#include <filesystem>
#include <gtest/gtest.h>

#include "common/util/defer.hpp"


#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#include <random>

using namespace std::literals;

static void ReplaceAll(std::string &str, const std::string &from, const std::string &to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

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

static void AssertDomainState(virDomainPtr ptr, int expected) {
    int state;
    int ret = virDomainGetState(ptr, &state, nullptr, 0);
    ASSERT_EQ(ret, 0) << VirtLastErrorString();
    ASSERT_EQ(state, expected);
}

static std::string CopyKernelIso() {
    std::mt19937 rng(std::random_device{}());
    std::string random = "/tmp/bezos-test-"s + std::to_string(rng()) + ".iso";
    auto iso = GetEnvElement("MESON_PREFIX") + "/bezos-limine.iso";
    std::filesystem::copy_file(iso, random, std::filesystem::copy_options::overwrite_existing);

    return random;
}

static void KillDomain(virConnectPtr conn, const std::string &name) {
    virDomainPtr domain = virDomainLookupByName(conn, name.c_str());
    if (domain == nullptr) {
        return;
    }

    int ret = virDomainDestroy(domain);
    if (ret != 0) {
        std::cerr << "Failed to destroy domain: " << VirtLastErrorString() << std::endl;
    }

    ret = virDomainUndefine(domain);
    if (ret != 0) {
        std::cerr << "Failed to undefine domain: " << VirtLastErrorString() << std::endl;
    }
}

//
// Create a domain using a kernel image, launch it, and then destroy it.
//
TEST(LaunchTest, Launch) {
    virConnectPtr conn = virConnectOpen("qemu:///system");
    ASSERT_NE(conn, nullptr) << VirtLastErrorString();
    defer { virConnectClose(conn); };

    char *uri = virConnectGetURI(conn);
    ASSERT_NE(uri, nullptr) << VirtLastErrorString();
    defer { free(uri); };

    const char *hvType = virConnectGetType(conn);
    ASSERT_NE(hvType, nullptr) << VirtLastErrorString();

    auto name = GetTestDomain("Launch");

    // If a domain already exists for this test, kill it.
    KillDomain(conn, name);

    auto iso = CopyKernelIso();
    std::string xml = kDomainXml;
    ReplaceAll(xml, "$0", name);
    ReplaceAll(xml, "$1", iso);

    virDomainPtr domain = virDomainDefineXMLFlags(conn, xml.c_str(), VIR_DOMAIN_DEFINE_VALIDATE);
    ASSERT_NE(domain, nullptr) << VirtLastErrorString() << xml;
    defer { virDomainFree(domain); };

    int ret = virDomainCreate(domain);
    ASSERT_EQ(ret, 0) << VirtLastErrorString();

    virDomainInfo info;
    ret = virDomainGetInfo(domain, &info);
    ASSERT_EQ(ret, 0) << VirtLastErrorString();

    AssertDomainState(domain, VIR_DOMAIN_RUNNING);

    ret = virDomainDestroy(domain);
    ASSERT_EQ(ret, 0) << VirtLastErrorString();
}

static void SpamInjectNmi(virDomainPtr domain, int count) {
    for (int i = 0; i < count; i++) {
        int ret = virDomainInjectNMI(domain, 0);
        ASSERT_EQ(ret, 0) << VirtLastErrorString();
    }
}

TEST(LaunchTest, InjectNmi) {
    virConnectPtr conn = virConnectOpen("qemu:///system");
    ASSERT_NE(conn, nullptr) << VirtLastErrorString();
    defer { virConnectClose(conn); };

    char *uri = virConnectGetURI(conn);
    ASSERT_NE(uri, nullptr) << VirtLastErrorString();
    defer { free(uri); };

    const char *hvType = virConnectGetType(conn);
    ASSERT_NE(hvType, nullptr) << VirtLastErrorString();

    auto name = GetTestDomain("InjectNmi");

    // If a domain already exists for this test, kill it.
    KillDomain(conn, name);

    auto iso = CopyKernelIso();
    std::string xml = kDomainXml;
    ReplaceAll(xml, "$0", name);
    ReplaceAll(xml, "$1", iso);

    virDomainPtr domain = virDomainDefineXMLFlags(conn, xml.c_str(), VIR_DOMAIN_DEFINE_VALIDATE);
    ASSERT_NE(domain, nullptr) << VirtLastErrorString() << xml.c_str();
    defer { virDomainFree(domain); };

    int ret = virDomainCreate(domain);
    ASSERT_EQ(ret, 0) << VirtLastErrorString();

    virDomainInfo info;
    ret = virDomainGetInfo(domain, &info);
    ASSERT_EQ(ret, 0) << VirtLastErrorString();

    int state;
    ret = virDomainGetState(domain, &state, nullptr, 0);
    ASSERT_EQ(ret, 0) << VirtLastErrorString();

    AssertDomainState(domain, VIR_DOMAIN_RUNNING);

    SpamInjectNmi(domain, 25);

    std::this_thread::sleep_for(500ms);

    SpamInjectNmi(domain, 25);

    AssertDomainState(domain, VIR_DOMAIN_RUNNING);

    ret = virDomainDestroy(domain);
    ASSERT_EQ(ret, 0) << VirtLastErrorString();
}
