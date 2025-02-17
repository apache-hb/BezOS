#include <bezos/syscall.h>

extern "C" [[noreturn]] void ClientStart(const char *ArgsBegin, const char *ArgsEnd, uint64_t) {
    while (true) {
        OsThreadYield();
    }
}
