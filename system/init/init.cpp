#include <cstddef>
#include <cstdint>

#include <bezos/syscall.h>

extern "C" size_t strlen(const char *str) {
    size_t len = 0;
    while (*str++) {
        len++;
    }
    return len;
}

static void OsDebugLog(const char *begin, const char *end) {
    OsSystemCall(eOsCallDebugLog, (uint64_t)begin, (uint64_t)end, 0, 0);
}

static void OsDebugLog(const char *message) {
    const char *begin = message;
    const char *end = message + strlen(message);

    OsDebugLog(begin, end);
}

template<size_t N>
static OsStatus OpenFile(const char (&path)[N], OsFileHandle *OutHandle) {
    const char *begin = path;
    const char *end = path + N - 1;

    return OsFileOpen(begin, end, eOsFileRead, OutHandle);
}

// rdi: first argument
// rsi: last argument
// rdx: reserved
extern "C" [[noreturn]] void ClientStart(uint64_t, uint64_t, uint64_t) {
    OsFileHandle Handle = OS_FILE_INVALID;
    if (OsStatus status = OpenFile("Users\0Guest\0motd.txt", &Handle)) {
        (void)status;

        OsDebugLog("Failed to open file /Users/Guest/motd.txt");
        while (1) { }
        __builtin_unreachable();
    }

    char buffer[256];
    size_t read = SIZE_MAX;
    if (OsStatus status = OsFileRead(Handle, buffer, buffer + sizeof(buffer), &read)) {
        (void)status;

        OsDebugLog("Failed to read file /Users/Guest/motd.txt");
        while (1) { }
        __builtin_unreachable();
    }

    OsDebugLog("Reading file /Users/Guest/motd.txt");
    OsDebugLog(buffer, buffer + read);

    while (1) { }
    __builtin_unreachable();
}
