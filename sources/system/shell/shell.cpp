#include <bezos/facility/device.h>
#include <bezos/facility/threads.h>
#include <bezos/facility/fs.h>
#include <bezos/facility/vmem.h>
#include <bezos/facility/process.h>
#include <bezos/facility/handle.h>
#include <bezos/facility/debug.h>
#include <bezos/facility/clock.h>

#include <bezos/handle.h>
#include <bezos/status.h>
#include <bezos/subsystem/fs.h>
#include <bezos/subsystem/hid.h>
#include <bezos/subsystem/ddi.h>
#include <bezos/subsystem/identify.h>

#include <posix/ext/args.h>

#include "common/bezos.hpp"
#include "defer.hpp"

#include <flanterm.h>
#include <backends/fb.h>

#include <tlsf.h>

#include <ctype.h>
#include <string.h>
#include <iterator>
#include <string>
#include <string_view>
#include <chrono>
#include <format>

template<size_t N>
static void Assert(bool condition, const char (&message)[N]) {
    if (!condition) {
        os::debug_message(eOsLogError, "{}", message);

        while (true) {
            OsThreadYield();
        }
    }
}

template<size_t N>
static void AssertOsSuccess(OsStatus status, const char (&message)[N]) {
    if (status != OsStatusSuccess) {
        os::debug_message(eOsLogError, "{}. Status: {}", message, status);
        while (true) {
            OsThreadYield();
        }
    }
}

#define ASSERT_OS_SUCCESS(expr) AssertOsSuccess(expr, "shell.elf: Assertion failed: " #expr " != OsStatusSuccess")
#define ASSERT(expr) Assert(expr, #expr)

#define UTIL_NOCOPY(cls) \
    cls(const cls &) = delete; \
    cls &operator=(const cls &) = delete;

#define UTIL_NOMOVE(cls) \
    cls(cls &&) = delete; \
    cls &operator=(cls &&) = delete;

class StreamDevice {
    OsDeviceHandle mDevice;

public:
    UTIL_NOCOPY(StreamDevice);
    UTIL_NOMOVE(StreamDevice);

    OsDeviceHandle Handle() const { return mDevice; }

    StreamDevice(OsPath path) {
        OsDeviceCreateInfo createInfo {
            .Path = path,
            .InterfaceGuid = kOsStreamGuid,
            .Flags = eOsDeviceOpenExisting,
        };

        ASSERT_OS_SUCCESS(OsDeviceOpen(createInfo, &mDevice));
    }

    ~StreamDevice() {
        ASSERT_OS_SUCCESS(OsDeviceClose(mDevice));
    }

    OsStatus Write(const char *front, const char *back) {
        OsDeviceWriteRequest request {
            .BufferFront = front,
            .BufferBack = back,
            .Timeout = OS_TIMEOUT_INFINITE,
        };

        OsSize written = 0;
        return OsDeviceWrite(mDevice, request, &written);
    }

    OsStatus Read(char *front, char *back, size_t *size) {
        OsDeviceReadRequest request {
            .BufferFront = front,
            .BufferBack = back,
            .Timeout = OS_TIMEOUT_INSTANT,
        };

        return OsDeviceRead(mDevice, request, size);
    }

    template<typename... A>
    OsStatus Format(const std::format_string<A...> &fmt, A&&... args) {
        auto text = std::vformat(fmt.get(), std::make_format_args(args...));
        return Write(text.data(), text.data() + text.size());
    }
};

template<size_t N>
static OsStatus OpenFile(const char (&path)[N], OsDeviceHandle *outHandle) {
    OsDeviceCreateInfo createInfo {
        .Path = OsMakePath(path),
        .InterfaceGuid = kOsFileGuid,
        .Flags = eOsDeviceOpenExisting,
    };

    return OsDeviceOpen(createInfo, outHandle);
}

static void Prompt(StreamDevice& tty) {
    tty.Format("root@localhost: ");
}

class FolderIterator {
    OsDeviceHandle mHandle;

public:
    UTIL_NOCOPY(FolderIterator);

    FolderIterator() : mHandle(OS_HANDLE_INVALID) {}

    FolderIterator(FolderIterator &&other) : mHandle(other.mHandle) {
        other.mHandle = OS_HANDLE_INVALID;
    }

    FolderIterator& operator=(FolderIterator &&other) {
        std::swap(mHandle, other.mHandle);
        return *this;
    }

    ~FolderIterator() {
        if (mHandle != OS_HANDLE_INVALID) {
            OsDeviceClose(mHandle);
        }
    }

    static OsStatus Create(OsPath path, FolderIterator *iterator) {
        OsDeviceCreateInfo createInfo {
            .Path = path,
            .InterfaceGuid = kOsIteratorGuid,
            .Flags = eOsDeviceOpenExisting,
        };

        return OsDeviceOpen(createInfo, &iterator->mHandle);
    }

    OsStatus Next(OsNodeHandle *outNode) {
        OsIteratorNext request {
            .Node = OS_HANDLE_INVALID,
        };

        if (OsStatus status = OsInvokeIteratorNext(mHandle, &request)) {
            return status;
        }

        *outNode = request.Node;
        return OsStatusSuccess;
    }
};

static void ListCurrentFolder(StreamDevice& tty, std::string_view path) {
    char copy[1024]{};
    memcpy(copy, path.data(), sizeof(copy));
    for (char *ptr = copy; *ptr != '\0'; ptr++) {
        if (*ptr == '/') {
            *ptr = '\0';
        }
    }

    size_t front = 0;
    size_t len = path.size();

    if (copy[0] == '\0') {
        front += 1;
    }

    OsPath iteratorPath = { copy + front, copy + len };

    if (path == "/") {
        iteratorPath = OsMakePath("");
    }

    FolderIterator iterator{};
    if (OsStatus status = FolderIterator::Create(iteratorPath, &iterator)) {
        tty.Format("Error: Failed to open folder '{}'.\n", path);
        tty.Format("Status: {}\n", status);
        return;
    }

    while (true) {
        OsNodeHandle node = OS_HANDLE_INVALID;
        defer {
            if (node != OS_HANDLE_INVALID) {
                ASSERT_OS_SUCCESS(OsNodeClose(node));
            }
        };

        if (OsStatus status = iterator.Next(&node)) {
            if (status == OsStatusCompleted) {
                break;
            }

            tty.Format("Error: Failed to iterate folder '{}'.\n", path);
            tty.Format("Status: {}\n", status);
            return;
        }

        OsNodeInfo info{};
        if (OsStatus status = OsNodeStat(node, &info)) {
            tty.Format("Error: Failed to get node info.\n");
            tty.Format("Status: {}\n", status);
            return;
        }

        std::string_view name = { info.Name, strnlen(info.Name, sizeof(info.Name)) };
        tty.Format("{}\n", name);
    }
}

static bool VfsNodeExists(std::string_view path, std::string_view cwd) {
    std::string copy;
    if (path[0] == '/' || cwd == "/") {
        copy = std::string(path);
    } else {
        copy = std::format("{}/{}", cwd, path);
    }

    size_t front = 0;

    for (char& c : copy) {
        if (c == '/') {
            c = '\0';
        }
    }

    if (copy[0] == '\0') {
        front += 1;
    }

    OsDeviceCreateInfo createInfo {
        .Path = { copy.data() + front, copy.data() + copy.size() },
        .InterfaceGuid = kOsIdentifyGuid,
    };

    if (path == "/") {
        createInfo.Path = OsMakePath("");
    }

    OsDeviceHandle handle = OS_HANDLE_INVALID;
    OsStatus status = OsDeviceOpen(createInfo, &handle);
    if (status != OsStatusSuccess) {
        return false;
    }

    ASSERT_OS_SUCCESS(OsDeviceClose(handle));
    return true;
}

static void SetCurrentFolder(StreamDevice& tty, std::string_view path, std::string& cwd) {
    if (!VfsNodeExists(path, cwd)) {
        tty.Format("Path '{}' does not exist.\n", path);
    } else {
        if (path[0] == '/' || cwd == "/") {
            cwd = path;
        } else {
            cwd += "/";
            cwd += path;
        }
    }
}

static void ShowCurrentInfo(StreamDevice& display, std::string_view cwd) {
    std::string copy = std::string(cwd);
    for (char& c : copy) {
        if (c == '/') {
            c = '\0';
        }
    }

    size_t front = 0;

    if (copy[0] == '\0') {
        front += 1;
    }

    OsDeviceCreateInfo createInfo {
        .Path = { copy.data() + front, copy.data() + copy.size() },
        .InterfaceGuid = kOsIdentifyGuid,
    };

    OsDeviceHandle handle = OS_HANDLE_INVALID;
    ASSERT_OS_SUCCESS(OsDeviceOpen(createInfo, &handle));

    OsIdentifyInfo info{};
    ASSERT_OS_SUCCESS(OsInvokeIdentifyDeviceInfo(handle, &info));

    display.Format("Name = '{}'\n", info.DisplayName);
    display.Format("Model = '{}'\n", info.Model);
    display.Format("Serial = '{}'\n", info.Serial);
    display.Format("DeviceVendor = '{}'\n", info.DeviceVendor);
    display.Format("FirmwareRevision = '{}'\n", info.FirmwareRevision);
    display.Format("DriverVendor = '{}'\n", info.DriverVendor);
    display.Format("DriverVersion = {}.{}.{}\n",
        OS_VERSION_MAJOR(info.DriverVersion),
        OS_VERSION_MINOR(info.DriverVersion),
        OS_VERSION_PATCH(info.DriverVersion)
    );

    ASSERT_OS_SUCCESS(OsDeviceClose(handle));
}

static void RunProgram(StreamDevice& tyy, OsDeviceHandle in, OsDeviceHandle out, OsDeviceHandle err, std::string_view path, std::string_view cwd) {
    struct CreateOptions {
        OsProcessParamHeader param;
        OsPosixInitArgs args;
    };

    CreateOptions options[] = { {
        .param = { kPosixInitGuid, sizeof(OsPosixInitArgs) },
        .args = {
            .StandardIn = in,
            .StandardOut = out,
            .StandardError = err,
        },
    } };

    std::string copy = std::string(cwd);

    if (path[0] == '/' || cwd == "/") {
        copy = path;
    } else {
        copy += "/";
        copy += path;
    }

    for (char& c : copy) {
        if (c == '/') {
            c = '\0';
        }
    }

    size_t front = 0;
    size_t len = path.size();

    if (copy[0] == '\0') {
        front += 1;
    }

    OsPath osPath = { copy.data() + front, copy.data() + len };

    if (path == "/") {
        osPath = OsMakePath("");
    }

    OsProcessCreateInfo createInfo {
        .Executable = osPath,
        .ArgsBegin = std::bit_cast<const OsProcessParam*>(std::begin(options)),
        .ArgsEnd = std::bit_cast<const OsProcessParam*>(std::end(options)),
        .Flags = eOsProcessNone,
    };

    OsProcessHandle handle = OS_HANDLE_INVALID;
    OsStatus status = OsStatusSuccess;

    status = OsProcessCreate(createInfo, &handle);
    if (status != OsStatusSuccess) {
        tyy.Format("Failed to create process: {}\n", status);
        return;
    }

    status = OsHandleWait(handle, OS_TIMEOUT_INFINITE);
    ASSERT_OS_SUCCESS(status);

    OsProcessInfo stat{};
    status = OsProcessStat(handle, &stat);
    if (status != OsStatusSuccess) {
        tyy.Format("Failed to stat process: {}\n", status);
        return;
    }

    tyy.Format("Process exited with code {}\n", stat.ExitCode);
}

static void LaunchZsh(StreamDevice& tty, OsDeviceHandle in, OsDeviceHandle out, OsDeviceHandle err) {
    struct CreateOptions {
        OsProcessParamHeader param;
        OsPosixInitArgs args;
    };

    CreateOptions options[] = { {
        .param = { kPosixInitGuid, sizeof(OsPosixInitArgs) },
        .args = {
            .StandardIn = in,
            .StandardOut = out,
            .StandardError = err,
        },
    } };

    OsProcessCreateInfo createInfo {
        .Executable = OsMakePath("Init\0zsh.elf"),
        .ArgsBegin = std::bit_cast<const OsProcessParam*>(std::begin(options)),
        .ArgsEnd = std::bit_cast<const OsProcessParam*>(std::end(options)),
        .Flags = eOsProcessNone,
    };

    OsProcessHandle handle = OS_HANDLE_INVALID;
    OsStatus status = OsStatusSuccess;

    status = OsProcessCreate(createInfo, &handle);
    if (status != OsStatusSuccess) {
        tty.Format("Failed to create zsh process: {}\n", status);
        return;
    }

    status = OsHandleWait(handle, OS_TIMEOUT_INFINITE);
    ASSERT_OS_SUCCESS(status);

    OsProcessInfo stat{};
    status = OsProcessStat(handle, &stat);
    if (status != OsStatusSuccess) {
        tty.Format("Failed to stat zsh process: {}\n", status);
        return;
    }

    tty.Format("zsh exited with code {}\n", stat.ExitCode);
}

static void EchoFile(StreamDevice& tty, std::string_view cwd, const char *path) {
    char copy[1024]{};

    if (path[0] == '/') {
        strcpy(copy, path);
    } else if (cwd == "/") {
        strcpy(copy, path);
    } else {
        strncpy(copy, cwd.data(), cwd.size());
        strcat(copy, "/");
        strcat(copy, path);
    }

    size_t front = 0;
    size_t len = strlen(copy);

    for (char& c : copy) {
        if (c == '/') {
            c = '\0';
        }
    }

    if (copy[0] == '\0') {
        front += 1;
    }

    OsDeviceHandle handle = OS_HANDLE_INVALID;
    OsDeviceCreateInfo createInfo {
        .Path = { copy + front, copy + len },
        .InterfaceGuid = kOsFileGuid,
    };

    if (strncmp(path, "/", 2) == 0) {
        createInfo.Path = OsMakePath("");
    }

    OsStatus status = OsDeviceOpen(createInfo, &handle);
    if (status != OsStatusSuccess) {
        tty.Format("Error failed to open: '{}': {}\n", path, status);
        return;
    }

    char buffer[1024];
    OsSize read = 0;
    while (true) {
        OsDeviceReadRequest request {
            .BufferFront = std::begin(buffer),
            .BufferBack = std::end(buffer),
            .Timeout = OS_TIMEOUT_INFINITE,
        };
        status = OsDeviceRead(handle, request, &read);
        if (status != OsStatusSuccess) {
            tty.Format("Error failed to read: '{}': {}\n", path, status);
            break;
        }

        if (read == 0) {
            break;
        }

        tty.Format("{}", std::string_view(buffer, buffer + read));
    }

    ASSERT_OS_SUCCESS(OsDeviceClose(handle));
}

using namespace std::chrono_literals;

static constexpr auto kGregorianReform = std::chrono::years{1582} + std::chrono::months{10} + std::chrono::days{15};
using os_instant = std::chrono::duration<OsInstant, std::ratio<1LL, 10000000LL>>;

static void WriteTime(StreamDevice& tty) {
    OsClockInfo info{};
    ASSERT_OS_SUCCESS(OsClockStat(&info));

    OsInstant time = 0;
    ASSERT_OS_SUCCESS(OsClockGetTime(&time));

    {
        os_instant timepoint{time};
        auto instant = timepoint + kGregorianReform;

        auto years = std::chrono::floor<std::chrono::years>(instant);
        auto months = std::chrono::floor<std::chrono::months>(instant - years);
        auto days = std::chrono::floor<std::chrono::days>(instant - years - months);
        auto hours = std::chrono::floor<std::chrono::hours>(instant - years - months - days);
        auto minutes = std::chrono::floor<std::chrono::minutes>(instant - years - months - days - hours);
        auto seconds = std::chrono::floor<std::chrono::seconds>(instant - years - months - days - hours - minutes);

        tty.Format("Current Time: {}-{}-{}T{}:{}:{}Z\n", years.count(), months.count(), days.count(), hours.count(), minutes.count(), seconds.count());
    }

    {
        os_instant timepoint{time};
        auto instant = timepoint + kGregorianReform;

        auto years = std::chrono::floor<std::chrono::years>(instant);
        auto months = std::chrono::floor<std::chrono::months>(instant - years);
        auto days = std::chrono::floor<std::chrono::days>(instant - years - months);
        auto hours = std::chrono::floor<std::chrono::hours>(instant - years - months - days);
        auto minutes = std::chrono::floor<std::chrono::minutes>(instant - years - months - days - hours);
        auto seconds = std::chrono::floor<std::chrono::seconds>(instant - years - months - days - hours - minutes);

        tty.Format("Boot Time: {}-{}-{}T{}:{}:{}Z\n", years.count(), months.count(), days.count(), hours.count(), minutes.count(), seconds.count());
    }

    tty.Format("Clock Frequency: {} Hz\n", info.FrequencyHz);
    tty.Format("Clock: '{}'\n", info.DisplayName);
}

OS_EXTERN OS_NORETURN
[[gnu::force_align_arg_pointer]]
void ClientStart(const struct OsClientStartInfo *) {
    StreamDevice tty{OsMakePath("Devices\0Terminal\0TTY0\0Output")};
    StreamDevice ttyin{OsMakePath("Devices\0Terminal\0TTY0\0Input")};

    {
        OsDeviceHandle Handle = OS_HANDLE_INVALID;
        ASSERT_OS_SUCCESS(OpenFile("Users\0Guest\0motd.txt", &Handle));

        char buffer[256];
        OsDeviceReadRequest request {
            .BufferFront = std::begin(buffer),
            .BufferBack = std::end(buffer),
            .Timeout = OS_TIMEOUT_INFINITE,
        };
        OsSize size = 0;
        ASSERT_OS_SUCCESS(OsDeviceRead(Handle, request, &size));

        tty.Write(buffer, buffer + size);
    }

    Prompt(tty);

    std::string text;
    std::string cwd = "/Users/Guest";

    free(malloc(1)); // TODO: stupid hack, remove this once i debug malloc in posix sysapi

    auto addChar = [&](char c) {
        text.push_back(c);
        tty.Format("{}", c);
    };

    while (true) {
        size_t size = 0;
        char input[1];
        if (ttyin.Read(input, input + 1, &size) != OsStatusSuccess || size == 0) {
            continue;
        }

        char key = input[0];
        if (key == '\n') {
            if (text.empty()) {
                tty.Format("\n");
                Prompt(tty);
                continue;
            }

            defer { text.clear(); };

            tty.Format("\n");

            if (text == "exit") {
                break;
            } else if (text == "uname") {
                tty.Format("BezOS localhost 0.0.1 amd64\n");
                Prompt(tty);
                continue;
            } else if (text == "pwd") {
                tty.Format("{}\n", cwd);
                Prompt(tty);
                continue;
            } else if (text == "ls") {
                ListCurrentFolder(tty, cwd);
                Prompt(tty);
                continue;
            } else if (text.starts_with("cd")) {
                SetCurrentFolder(tty, text.substr(3), cwd);
                Prompt(tty);
                continue;
            } else if (text == "show") {
                ShowCurrentInfo(tty, cwd);
                Prompt(tty);
                continue;
            } else if (text == "zsh") {
                LaunchZsh(tty, ttyin.Handle(), tty.Handle(), tty.Handle());
                Prompt(tty);
                continue;
            } else if (text.starts_with("cat")) {
                EchoFile(tty, cwd, text.substr(4).c_str());
                Prompt(tty);
                continue;
            } else if (text.starts_with("exec")) {
                RunProgram(tty, ttyin.Handle(), tty.Handle(), tty.Handle(), text.substr(5), cwd);
                Prompt(tty);
                continue;
            } else if (text.starts_with("stress")) {
                for (int i = 0; i < 1000; i++) {
                    RunProgram(tty, ttyin.Handle(), tty.Handle(), tty.Handle(), "/Init/test.elf", cwd);
                }
                Prompt(tty);
                continue;
            } else if (text.starts_with("date")) {
                WriteTime(tty);
                Prompt(tty);
                continue;
            }

            tty.Format("Unknown command: '{}'\n", text);
            Prompt(tty);
        } else if (key == '\b') {
            if (!text.empty()) {
                text.pop_back();
                tty.Format("\b \b");
            }
        } else if (isprint(key)) {
            addChar(key);
        }
    }

    tty.Format("Shell exited\n");
    while (true) { }
    __builtin_unreachable();
}
