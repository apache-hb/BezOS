#include <bezos/facility/device.h>
#include <bezos/facility/threads.h>
#include <bezos/facility/fs.h>
#include <bezos/facility/vmem.h>
#include <bezos/facility/process.h>

#include <bezos/subsystem/hid.h>
#include <bezos/subsystem/ddi.h>

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

#define ASSERT_OS_SUCCESS(expr) AssertOsSuccess(expr, "Assertion failed: " #expr " != OsStatusSuccess")

extern "C" [[noreturn]] void ClientStart() {
    char path[] = "Init\0shell.elf";
    char args[] = "";

    OsProcessCreateInfo createInfo {
        .Executable = { std::begin(path), std::end(path) - 1 },

        .ArgumentsBegin = std::begin(args),
        .ArgumentsEnd = std::end(args) - 1,

        .Flags = eOsProcessNone,
    };

    OsProcessHandle handle = OS_HANDLE_INVALID;
    ASSERT_OS_SUCCESS(OsProcessCreate(createInfo, &handle));

    while (true) {
        OsThreadYield();
    }
}
