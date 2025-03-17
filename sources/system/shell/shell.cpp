#include <bezos/facility/device.h>
#include <bezos/facility/threads.h>
#include <bezos/facility/fs.h>
#include <bezos/facility/vmem.h>
#include <bezos/facility/process.h>
#include <bezos/facility/handle.h>
#include <bezos/facility/debug.h>

#include <bezos/handle.h>
#include <bezos/status.h>
#include <bezos/subsystem/fs.h>
#include <bezos/subsystem/hid.h>
#include <bezos/subsystem/ddi.h>
#include <bezos/subsystem/identify.h>

#include <posix/ext/args.h>

#include "defer.hpp"

#include <concepts>
#include <flanterm.h>
#include <backends/fb.h>

#include <tlsf.h>

#include <ctype.h>
#include <string.h>
#include <iterator>
#include <string>
#include <string_view>
#include <format>

template<size_t N>
static void DebugLog(const char (&message)[N]) {
    OsDebugMessage(eOsLogDebug, message);
}

template<typename... A>
static void FormatLog(const std::format_string<A...>& fmt, A&&... args) {
    auto text = std::vformat(fmt.get(), std::make_format_args(args...));

    OsDebugMessageInfo messageInfo {
        .Front = text.data(),
        .Back = text.data() + text.size(),
        .Info = eOsLogDebug,
    };

    OsDebugMessage(messageInfo);
}

template<typename T>
static void DebugLog(T number) {
    char buffer[32];
    char *ptr = buffer + sizeof(buffer);
    *--ptr = '\0';

    if (number == 0) {
        *--ptr = '0';
    } else {
        while (number != 0) {
            *--ptr = '0' + (number % 10);
            number /= 10;
        }
    }

    OsDebugMessage({ ptr, buffer + sizeof(buffer), eOsLogDebug });
}

template<size_t N>
static void Assert(bool condition, const char (&message)[N]) {
    if (!condition) {
        DebugLog(message);

        while (true) {
            OsThreadYield();
        }
    }
}

template<size_t N>
static void AssertOsSuccess(OsStatus status, const char (&message)[N]) {
    if (status != OsStatusSuccess) {
        DebugLog(message);
        DebugLog("Status: ");
        DebugLog(status);
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

        ASSERT_OS_SUCCESS(OsDeviceOpen(createInfo, NULL, 0, &mDevice));
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

static OsStatus WriteString(StreamDevice& stream, const char *begin, const char *end) {
    return stream.Write(begin, end);
}

template<size_t N>
static OsStatus WriteString(StreamDevice& stream, const char (&str)[N]) {
    return WriteString(stream, str, str + N - 1);
}

template<std::integral T>
void WriteNumber(StreamDevice& stream, T number) {
    char buffer[32];
    char *ptr = buffer + sizeof(buffer);
    *--ptr = '\0';

    if (number == 0) {
        *--ptr = '0';
    } else {
        while (number != 0) {
            *--ptr = '0' + (number % 10);
            number /= 10;
        }
    }

    WriteString(stream, ptr, buffer + sizeof(buffer));
}

template<size_t N>
static OsStatus OpenFile(const char (&path)[N], OsFileHandle *outHandle) {
    OsFileCreateInfo createInfo {
        .Path = OsMakePath(path),
        .Mode = eOsFileRead,
    };

    return OsFileOpen(createInfo, outHandle);
}

static void Prompt(StreamDevice& tty) {
    WriteString(tty, "root@localhost: ");
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

        return OsDeviceOpen(createInfo, NULL, 0, &iterator->mHandle);
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
        WriteString(tty, "Error: '");
        WriteString(tty, path.data(), path.data() + path.size());
        WriteString(tty, "' is not iterable.\n");
        WriteString(tty, "Status: ");
        WriteNumber(tty, status);
        WriteString(tty, "\n");

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

            WriteString(tty, "Error: Failed to iterate folder.\n");
            WriteString(tty, "Status: ");
            WriteNumber(tty, status);
            WriteString(tty, "\n");
            return;
        }

        OsNodeInfo info{};
        if (OsStatus status = OsNodeStat(node, &info)) {
            WriteString(tty, "Error: Failed to get node info.\n");
            WriteString(tty, "Status: ");
            WriteNumber(tty, status);
            WriteString(tty, "\n");
            return;
        }

        WriteString(tty, info.Name, info.Name + strnlen(info.Name, sizeof(info.Name)));
        WriteString(tty, "\n");
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
    OsStatus status = OsDeviceOpen(createInfo, NULL, 0, &handle);
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
    ASSERT_OS_SUCCESS(OsDeviceOpen(createInfo, NULL, 0, &handle));

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

    OsHandle handle = OS_HANDLE_INVALID;
    OsFileCreateInfo createInfo {
        .Path = { copy + front, copy + len },
        .Mode = eOsFileRead | eOsFileOpenExisting,
    };

    if (strncmp(path, "/", 2) == 0) {
        createInfo.Path = OsMakePath("");
    }

    OsStatus status = OsFileOpen(createInfo, &handle);
    if (status != OsStatusSuccess) {
        WriteString(tty, "Error failed to open: '");
        WriteString(tty, path, path + strlen(path));
        WriteString(tty, "' ");
        WriteNumber(tty, status);
        WriteString(tty, "\n");
        return;
    }

    char buffer[1024];
    size_t read = 0;
    while (true) {
        status = OsFileRead(handle, buffer, buffer + sizeof(buffer), &read);
        if (status != OsStatusSuccess) {
            WriteString(tty, "Error failed to read: '");
            WriteString(tty, path, path + strlen(path));
            WriteString(tty, "' ");
            WriteNumber(tty, status);
            WriteString(tty, "\n");
            break;
        }

        if (read == 0) {
            break;
        }

        WriteString(tty, buffer, buffer + read);
    }

    ASSERT_OS_SUCCESS(OsFileClose(handle));
}

OS_EXTERN OS_NORETURN
[[gnu::force_align_arg_pointer]]
void ClientStart(const struct OsClientStartInfo *) {
    StreamDevice tty{OsMakePath("Devices\0Terminal\0TTY0\0Output")};
    StreamDevice ttyin{OsMakePath("Devices\0Terminal\0TTY0\0Input")};

    {
        OsFileHandle Handle = OS_FILE_INVALID;
        ASSERT_OS_SUCCESS(OpenFile("Users\0Guest\0motd.txt", &Handle));

        char buffer[256];
        uint64_t read = 0;
        ASSERT_OS_SUCCESS(OsFileRead(Handle, std::begin(buffer), std::end(buffer), &read));

        tty.Write(buffer, buffer + read);
    }

    Prompt(tty);

    std::string text;
    std::string cwd = "/Users/Guest";

    auto addChar = [&](char c) {
        text.push_back(c);
        char buffer[1] = { c };
        WriteString(tty, buffer, buffer + 1);
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
                WriteString(tty, "\n");
                Prompt(tty);
                continue;
            }

            defer { text.clear(); };

            WriteString(tty, "\n");

            if (text == "exit") {
                break;
            } else if (text == "uname") {
                WriteString(tty, "BezOS localhost 0.0.1 amd64\n");
                Prompt(tty);
                continue;
            } else if (text == "pwd") {
                WriteString(tty, cwd.data(), cwd.data() + cwd.size());
                WriteString(tty, "\n");
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
            }

            tty.Format("Unknown command: '{}'\n", text);
            Prompt(tty);
        } else if (key == '\b') {
            if (!text.empty()) {
                text.pop_back();
                WriteString(tty, "\b \b");
            }
        } else if (isprint(key)) {
            addChar(key);
        }
    }

    WriteString(tty, "Shell exited\n");
    while (true) { }
    __builtin_unreachable();
}
