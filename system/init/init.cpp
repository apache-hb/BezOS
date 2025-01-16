#include <cstdint>

extern "C" uint64_t OsSystemCall(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

enum SystemCall : uint64_t {
    eExitProcess = 0,
};

// rdi: first argument
// rsi: last argument
// rdx: reserved
extern "C" [[noreturn]] void ClientStart(uint64_t, uint64_t, uint64_t) {
    uint64_t result = OsSystemCall(0x1234, 1, 2, 3, 4);
    OsSystemCall(eExitProcess, result, 0, 0, 0);
    while (1) { }
    __builtin_unreachable();
}
