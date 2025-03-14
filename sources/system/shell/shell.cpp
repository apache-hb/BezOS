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

#include <concepts>
#include <flanterm.h>
#include <backends/fb.h>

#include <tlsf.h>

#include <ctype.h>
#include <string.h>
#include <iterator>

template<size_t N>
static void DebugLog(const char (&message)[N]) {
    OsDebugMessage(eOsLogDebug, message);
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
        DebugLog("\nStatus: ");
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
    FolderIterator() : mHandle(OS_HANDLE_INVALID) {}

    FolderIterator(FolderIterator &&other) : mHandle(other.mHandle) {
        other.mHandle = OS_HANDLE_INVALID;
    }

    FolderIterator& operator=(FolderIterator &&other) {
        std::swap(mHandle, other.mHandle);
        return *this;
    }

    FolderIterator(const FolderIterator&) = delete;
    FolderIterator& operator=(const FolderIterator&) = delete;

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

static void ListCurrentFolder(StreamDevice& tty, const char *path) {
    char copy[1024]{};
    memcpy(copy, path, sizeof(copy));
    for (char *ptr = copy; *ptr != '\0'; ptr++) {
        if (*ptr == '/') {
            *ptr = '\0';
        }
    }

    size_t front = 0;
    size_t len = strlen(path);

    if (copy[0] == '\0') {
        front += 1;
    }

    OsPath iteratorPath = { copy + front, copy + len };

    if (strncmp(path, "/", 2) == 0) {
        iteratorPath = OsMakePath("");
    }

    FolderIterator iterator{};
    if (OsStatus status = FolderIterator::Create(iteratorPath, &iterator)) {
        WriteString(tty, "Error: '");
        WriteString(tty, path, path + strlen(path));
        WriteString(tty, "' is not iterable.\n");
        WriteString(tty, "Status: ");
        WriteNumber(tty, status);
        WriteString(tty, "\n");

        return;
    }

    while (true) {
        OsNodeHandle node = OS_HANDLE_INVALID;
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

static bool VfsNodeExists(const char *path, const char *cwd) {
    char copy[1024]{};

    if (path[0] == '/') {
        strcpy(copy, path);
    } else if (strncmp(cwd, "/", 2) == 0) {
        strcpy(copy, path);
    } else {
        strcpy(copy, cwd);
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

    OsDeviceCreateInfo createInfo {
        .Path = { copy + front, copy + len },
        .InterfaceGuid = kOsIdentifyGuid,
    };

    if (strncmp(path, "/", 2) == 0) {
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

static constexpr size_t kCwdSize = 1024;

static void SetCurrentFolder(StreamDevice& tty, const char *path, char *cwd) {
    if (!VfsNodeExists(path, cwd)) {
        WriteString(tty, "Path '");
        WriteString(tty, path, path + strlen(path));
        WriteString(tty, "' does not exist.\n");
    } else {
        if (path[0] == '/') {
            strncpy(cwd, path, kCwdSize);
        } else if (strncmp(cwd, "/", 2) == 0) {
            strcat(cwd, path);
        } else {
            strcat(cwd, "/");
            strcat(cwd, path);
        }

        size_t len = strlen(cwd);
        char *end = cwd + len;
        size_t size = kCwdSize - len;
        memset(end, 0, size);
    }
}

static void ShowCurrentInfo(StreamDevice& display, const char *cwd) {
    char copy[1024];
    memcpy(copy, cwd, sizeof(copy));
    for (char *ptr = copy; *ptr != '\0'; ptr++) {
        if (*ptr == '/') {
            *ptr = '\0';
        }
    }

    size_t front = 0;
    size_t len = strlen(cwd);

    if (copy[0] == '\0') {
        front += 1;
    }

    OsDeviceCreateInfo createInfo {
        .Path = { copy + front, copy + len },
        .InterfaceGuid = kOsIdentifyGuid,
    };

    OsDeviceHandle handle = OS_HANDLE_INVALID;
    ASSERT_OS_SUCCESS(OsDeviceOpen(createInfo, NULL, 0, &handle));

    OsIdentifyInfo info{};
    ASSERT_OS_SUCCESS(OsInvokeIdentifyDeviceInfo(handle, &info));

    WriteString(display, "Name = ");
    WriteString(display, info.DisplayName, info.DisplayName + strlen(info.DisplayName));
    WriteString(display, "\n");

    WriteString(display, "Model = ");
    WriteString(display, info.Model, info.Model + strlen(info.Model));
    WriteString(display, "\n");

    WriteString(display, "Serial = ");
    WriteString(display, info.Serial, info.Serial + strlen(info.Serial));
    WriteString(display, "\n");

    WriteString(display, "DeviceVendor = ");
    WriteString(display, info.DeviceVendor, info.DeviceVendor + strlen(info.DeviceVendor));
    WriteString(display, "\n");

    WriteString(display, "FirmwareRevision = ");
    WriteString(display, info.FirmwareRevision, info.FirmwareRevision + strlen(info.FirmwareRevision));
    WriteString(display, "\n");

    WriteString(display, "DriverVendor = ");
    WriteString(display, info.DriverVendor, info.DriverVendor + strlen(info.DriverVendor));
    WriteString(display, "\n");

    WriteString(display, "DriverVersion = ");
    WriteNumber(display, OS_VERSION_MAJOR(info.DriverVersion));
    WriteString(display, ".");
    WriteNumber(display, OS_VERSION_MINOR(info.DriverVersion));
    WriteString(display, ".");
    WriteNumber(display, OS_VERSION_PATCH(info.DriverVersion));
    WriteString(display, "\n");

    ASSERT_OS_SUCCESS(OsDeviceClose(handle));
}

static void LaunchZsh() {
    OsProcessCreateInfo createInfo {
        .Executable = OsMakePath("Init\0zsh.elf"),
        .Flags = eOsProcessNone,
    };

    OsProcessHandle handle = OS_HANDLE_INVALID;
    OsStatus status = OsStatusSuccess;

    status = OsProcessCreate(createInfo, &handle);
    if (status != OsStatusSuccess) {
        DebugLog("Failed to create zsh process\n");
        DebugLog("Status: ");
        DebugLog(status);
        return;
    }

    //
    // Create stdin, stdout, and stderr files in the new processes runfs.
    //

    #if 0
    //
    // Resume the process now that it has the necessary environment.
    //
    status = OsProcessSuspend(handle, false);
    if (status != OsStatusSuccess) {
        DebugLog("Failed to resume zsh process\n");
        DebugLog("Status: ");
        DebugLog(status);
        return;
    }
    #endif

    status = OsHandleWait(handle, OS_TIMEOUT_INFINITE);
    ASSERT_OS_SUCCESS(status);

    #if 0
    OsProcessState state{};
    status = OsProcessStat(handle, &state);
    if (status != OsStatusSuccess) {
        DebugLog("Failed to stat zsh process\n");
        DebugLog("Status: ");
        DebugLog(status);
        return;
    }

    DebugLog("zsh exited with code ");
    DebugLog(state.ExitCode);
    DebugLog("\n");
    #endif
}

static void EchoFile(StreamDevice& tty, const char *cwd, const char *path) {
    char copy[1024]{};

    if (path[0] == '/') {
        strcpy(copy, path);
    } else if (strncmp(cwd, "/", 2) == 0) {
        strcpy(copy, path);
    } else {
        strcpy(copy, cwd);
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

    char text[0x1000];
    size_t index = 0;

    char cwd[kCwdSize] = "/Users/Guest";

    auto addChar = [&](char c) {
        text[index++] = c;
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
            if (index == 0) {
                WriteString(tty, "\n");
                Prompt(tty);
                continue;
            }

            size_t oldIndex = index;
            text[index] = '\0';
            index = 0;

            WriteString(tty, "\n");

            if (strcmp(text, "exit") == 0) {
                break;
            } else if (strncmp(text, "uname", 5) == 0) {
                WriteString(tty, "BezOS localhost 0.0.1 amd64\n");
                Prompt(tty);
                continue;
            } else if (strncmp(text, "pwd", 3) == 0) {
                WriteString(tty, cwd, cwd + strlen(cwd));
                WriteString(tty, "\n");
                Prompt(tty);
                continue;
            } else if (strncmp(text, "ls", 2) == 0) {
                ListCurrentFolder(tty, cwd);
                Prompt(tty);
                continue;
            } else if (strncmp(text, "cd", 2) == 0) {
                const char *path = text + 3;
                SetCurrentFolder(tty, path, cwd);
                Prompt(tty);
                continue;
            } else if (strncmp(text, "show", 4) == 0) {
                ShowCurrentInfo(tty, cwd);
                Prompt(tty);
                continue;
            } else if (strncmp(text, "zsh", 3) == 0) {
                LaunchZsh();
                Prompt(tty);
                continue;
            } else if (strncmp(text, "cat", 3) == 0) {
                EchoFile(tty, cwd, text + 4);
                Prompt(tty);
                continue;
            }

            WriteString(tty, "Unknown command: ");
            WriteString(tty, text, text + oldIndex);
            WriteString(tty, "\n");

            Prompt(tty);
        } else if (key == '\b') {
            if (index > 0) {
                index--;
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
