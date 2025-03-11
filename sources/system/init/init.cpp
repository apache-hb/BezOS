#include <bezos/facility/device.h>
#include <bezos/facility/threads.h>
#include <bezos/facility/fs.h>
#include <bezos/facility/vmem.h>
#include <bezos/facility/process.h>
#include <bezos/facility/debug.h>

#include <bezos/subsystem/fs.h>
#include <bezos/subsystem/hid.h>
#include <bezos/subsystem/ddi.h>

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

#define ASSERT_OS_SUCCESS(expr) AssertOsSuccess(expr, "init.elf: Assertion failed: " #expr " != OsStatusSuccess")

static bool DeviceExists(OsPath path, OsGuid guid) {
    OsDeviceCreateInfo createInfo {
        .Path = path,
        .InterfaceGuid = guid,
        .Flags = eOsDeviceOpenDefault,
    };

    OsDeviceHandle handle = OS_HANDLE_INVALID;
    OsStatus status = OsDeviceOpen(createInfo, NULL, 0, &handle);
    if (status != OsStatusSuccess) {
        return false;
    }

    ASSERT_OS_SUCCESS(OsDeviceClose(handle));
    return true;
}

static void LaunchShell() {
    OsProcessCreateInfo createInfo {
        .Executable = OsMakePath("Init\0shell.elf"),
        .Flags = eOsProcessNone,
    };

    OsProcessHandle handle = OS_HANDLE_INVALID;
    ASSERT_OS_SUCCESS(OsProcessCreate(createInfo, &handle));
}

static void LaunchTerminalService() {
    OsProcessCreateInfo createInfo {
        .Executable = OsMakePath("Init\0tty.elf"),
        .Flags = eOsProcessNone,
    };

    OsProcessHandle handle = OS_HANDLE_INVALID;
    ASSERT_OS_SUCCESS(OsProcessCreate(createInfo, &handle));
}

OS_EXTERN OS_NORETURN void ClientStart(const struct OsClientStartInfo *) {
    LaunchTerminalService();

    DebugLog("Waiting for TTY0 devices...");

    while (!DeviceExists(OsMakePath("Devices\0Terminal\0TTY0\0Input"), kOsStreamGuid)) {
        DebugLog("Waiting for TTY0 input device...");
        OsThreadYield();
    }

    while (!DeviceExists(OsMakePath("Devices\0Terminal\0TTY0\0Output"), kOsStreamGuid)) {
        DebugLog("Waiting for TTY0 output device...");
        OsThreadYield();
    }

    LaunchShell();
    while (true) {
        OsThreadYield();
    }
}
