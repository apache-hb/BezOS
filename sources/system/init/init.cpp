#include <bezos/facility/device.h>
#include <bezos/facility/threads.h>
#include <bezos/facility/fs.h>
#include <bezos/facility/vmem.h>
#include <bezos/facility/process.h>
#include <bezos/facility/debug.h>

#include <bezos/subsystem/fs.h>
#include <bezos/subsystem/hid.h>
#include <bezos/subsystem/ddi.h>

#include <rtld/rtld.h>

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
        DebugLog("Status: ");
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
        .Flags = eOsDeviceOpenExisting,
    };

    OsDeviceHandle handle = OS_HANDLE_INVALID;
    OsStatus status = OsDeviceOpen(createInfo, &handle);
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
    OsDeviceCreateInfo deviceCreateInfo {
        .Path = OsMakePath("Init\0tty.elf"),
        .InterfaceGuid = kOsFileGuid,
        .Flags = eOsDeviceOpenExisting,
    };

    OsProcessCreateInfo processCreateInfo {
        .Name = "TTY.ELF",
        .Flags = eOsProcessNone,
    };

    OsDeviceHandle device = OS_HANDLE_INVALID;
    OsProcessHandle process = OS_HANDLE_INVALID;
    OsThreadHandle thread = OS_HANDLE_INVALID;

    ASSERT_OS_SUCCESS(OsDeviceOpen(deviceCreateInfo, &device));
    ASSERT_OS_SUCCESS(OsProcessCreate(processCreateInfo, &process));

    RtldStartInfo startInfo {
        .Program = device,
        .Process = process,
        .ProgramName = "TTY.ELF",
    };

    ASSERT_OS_SUCCESS(RtldStartProgram(&startInfo, &thread));
    ASSERT_OS_SUCCESS(OsThreadSuspend(thread, false));
}

OS_EXTERN OS_NORETURN [[gnu::force_align_arg_pointer]] void ClientStart(const struct OsClientStartInfo *) {
    DebugLog("INIT.ELF: Starting...");

    LaunchTerminalService();

    DebugLog("Waiting for TTY0 devices...");

    DebugLog("Waiting for TTY0 input device...");
    while (!DeviceExists(OsMakePath("Devices\0Terminal\0TTY0\0Input"), kOsStreamGuid)) {
        OsThreadYield();
    }

    DebugLog("Waiting for TTY0 output device...");
    while (!DeviceExists(OsMakePath("Devices\0Terminal\0TTY0\0Output"), kOsStreamGuid)) {
        OsThreadYield();
    }

    LaunchShell();
    while (true) {
        OsThreadYield();
    }
}
