
#include <cstdint>

extern "C" void OsSystemCall(uint64_t, uint64_t, uint64_t, uint64_t);

enum SystemCall : uint64_t {
    eDebugPrint = 1,
    eQueryInterface = 0x1000,
    eExitProcess = 0x1001,
};

// rdi: first argument
// rsi: last argument
// rdx: reserved
extern "C" [[noreturn]] void ClientStart(uint64_t, uint64_t, uint64_t) {
    OsSystemCall(eDebugPrint, 0, 0, 0);

    OsSystemCall(eExitProcess, 0, 0, 0);

    while (1) { }
    __builtin_unreachable();
}
