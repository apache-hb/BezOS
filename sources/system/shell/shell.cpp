#include <bezos/facility/device.h>
#include <bezos/facility/threads.h>
#include <bezos/facility/fs.h>
#include <bezos/facility/vmem.h>

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
    OsDebugLog(message, message + N - 1);
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

    OsDebugLog(ptr, buffer + sizeof(buffer));
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

template<size_t N>
OsAnyPointer VirtualAllocate(OsSize size, OsMemoryAccess access, const char (&name)[N]) {
    OsAddressSpaceCreateInfo createInfo = {
        .NameFront = std::begin(name),
        .NameBack = std::end(name) - 1,

        .Size = size,
        .Access = access,
    };

    OsAddressSpaceHandle handle{};
    ASSERT_OS_SUCCESS(OsAddressSpaceCreate(createInfo, &handle));

    OsAddressSpaceInfo stat{};
    ASSERT_OS_SUCCESS(OsAddressSpaceStat(handle, &stat));

    return stat.Base;
}

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
            .Flags = eOsDeviceOpenAlways,
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

static void ListCurrentFolder(StreamDevice& tty, const char *path) {
    alignas(OsFolderEntry) char buffer[1024];

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

    OsFolderIterateCreateInfo createInfo {
        .Path = { copy + front, copy + len },
    };

    if (strncmp(path, "/", 2) == 0) {
        createInfo.Path = OsMakePath("");
    }

    OsFolderIteratorHandle handle = OS_FOLDER_ITERATOR_INVALID;
    if (OsStatus status = OsFolderIterateCreate(createInfo, &handle)) {
        WriteString(tty, "Error: '");
        WriteString(tty, path, path + strlen(path));
        WriteString(tty, "' is not a folder.\n");
        WriteString(tty, "Status: ");
        WriteNumber(tty, status);
        WriteString(tty, "\n");

        return;
    }

    OsFolderEntry *iter = reinterpret_cast<OsFolderEntry*>(buffer);

    while (true) {
        iter->NameSize = sizeof(buffer) - sizeof(OsFolderEntry);
        OsStatus status = OsFolderIterateNext(handle, iter);
        if (status == OsStatusEndOfFile) {
            break;
        }

        ASSERT_OS_SUCCESS(status);

        WriteString(tty, iter->Name, iter->Name + iter->NameSize);
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

OS_EXTERN OS_NORETURN void ClientStart(const struct OsClientStartInfo *) {

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
