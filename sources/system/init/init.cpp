#include <bezos/facility/device.h>
#include <bezos/facility/thread.h>
#include <bezos/facility/node.h>
#include <bezos/facility/vmem.h>
#include <bezos/facility/process.h>
#include <bezos/facility/debug.h>
#include <bezos/facility/handle.h>

#include <bezos/subsystem/fs.h>
#include <bezos/subsystem/hid.h>
#include <bezos/subsystem/ddi.h>

#include <ext/args.h>
#include <rtld/rtld.h>

#include <bit>
#include <iterator>

#include <string.h>

constexpr OsPath kInputPath = OsMakePath("Devices\0Terminal\0TTY0\0Input");
constexpr OsPath kOutputPath = OsMakePath("Devices\0Terminal\0TTY0\0Output");

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

static void LaunchProcess(OsPath path, OsProcessCreateInfo processCreateInfo, OsProcessHandle *result) {
    OsDeviceCreateInfo deviceCreateInfo {
        .Path = path,
        .InterfaceGuid = kOsFileGuid,
        .Flags = eOsDeviceOpenExisting,
    };

    OsDeviceHandle device = OS_HANDLE_INVALID;
    OsProcessHandle process = OS_HANDLE_INVALID;
    OsThreadHandle thread = OS_HANDLE_INVALID;

    ASSERT_OS_SUCCESS(OsDeviceOpen(deviceCreateInfo, &device));
    ASSERT_OS_SUCCESS(OsProcessCreate(processCreateInfo, &process));

    RtldStartInfo startInfo {
        .Program = device,
        .Process = process,
    };

    ASSERT_OS_SUCCESS(RtldStartProgram(&startInfo, &thread));
    ASSERT_OS_SUCCESS(OsThreadSuspend(thread, false));
    ASSERT_OS_SUCCESS(OsDeviceClose(device));
    ASSERT_OS_SUCCESS(OsHandleClose(thread));

    if (result) *result = process; else {
        ASSERT_OS_SUCCESS(OsHandleClose(process));
    }
}

static void LaunchShell() {
    OsProcessCreateInfo createInfo {
        .Name = "SHELL.ELF",
    };

    LaunchProcess(OsMakePath("Init\0shell.elf"), createInfo, nullptr);
}

static void LaunchTerminalService() {
    OsProcessCreateInfo createInfo {
        .Name = "TTY.ELF",
    };

    LaunchProcess(OsMakePath("Init\0tty.elf"), createInfo, nullptr);
}

OS_EXTERN [[noreturn, gnu::force_align_arg_pointer]] void ClientStart(const struct OsClientStartInfo *) {
    DebugLog("INIT.ELF: Starting...");

    LaunchTerminalService();

    DebugLog("Waiting for TTY0 devices...");

    DebugLog("Waiting for TTY0 input device...");
    while (!DeviceExists(kInputPath, kOsStreamGuid)) {
        OsThreadYield();
    }

    DebugLog("Waiting for TTY0 output device...");
    while (!DeviceExists(kOutputPath, kOsStreamGuid)) {
        OsThreadYield();
    }

    LaunchShell();
    while (true) {
        OsThreadYield();
    }
}
