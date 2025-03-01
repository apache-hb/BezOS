#include <bezos/facility/process.h>

#include <bezos/start.h>

extern "C" int CrtEntry(int argc, const char *const *argv) asm("main");

static const char *const kDefaultArgs[] = { "program.elf", nullptr };

extern "C" [[noreturn]] void ProgramMain(const OsClientStartInfo *StartInfo) {
    int result = CrtEntry(1, kDefaultArgs);

    OsProcessHandle ThisProcess = OS_HANDLE_INVALID;
    OsProcessCurrent(&ThisProcess);

    //
    // Terminate the process with the result of main. OsProcessTerminate is noreturn
    // when called with the current process handle.
    //
    OsProcessTerminate(ThisProcess, result);
    __builtin_unreachable();
}
