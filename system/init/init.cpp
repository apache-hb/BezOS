#include <cstddef>
#include <cstdint>

extern "C" uint64_t OsSystemCall(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

enum SystemCall : uint64_t {
    eExitProcess = 0,

    eSysOpen = 10,
    eSysClose = 11,
    eSysRead = 12,

    eSysLog = 20,
};

size_t strlen(const char *str) {
    size_t len = 0;
    while (*str++) {
        len++;
    }
    return len;
}

static void OsDebugLog(const char *begin, const char *end) {
    OsSystemCall(eSysLog, (uint64_t)begin, (uint64_t)end, 0, 0);
}

static void OsDebugLog(const char *message) {
    const char *begin = message;
    const char *end = message + strlen(message);

    OsDebugLog(begin, end);
}

template<size_t N>
static uint64_t OsOpenFile(const char (&path)[N]) {
    const char *begin = path;
    const char *end = path + N;

    return OsSystemCall(eSysOpen, (uint64_t)begin, (uint64_t)end, 0, 0);
}

static size_t OsReadFile(uint64_t fd, void *buffer, size_t size) {
    return OsSystemCall(eSysRead, fd, (uint64_t)buffer, size, 0);
}

// rdi: first argument
// rsi: last argument
// rdx: reserved
extern "C" [[noreturn]] void ClientStart(uint64_t, uint64_t, uint64_t) {
    uint64_t fd = OsOpenFile("Users\0Guest\0motd.txt");

    char buffer[256];
    size_t read = OsReadFile(fd, buffer, sizeof(buffer));

    OsDebugLog("Reading file /Users/Guest/motd.txt");
    OsDebugLog(buffer, buffer + read);

    while (1) { }
    __builtin_unreachable();
}
