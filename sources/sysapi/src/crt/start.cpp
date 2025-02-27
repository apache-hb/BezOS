#include <bezos/facility/process.h>

#include <bezos/start.h>

extern "C" int CrtEntry(int argc, char **argv) asm("main");

extern "C" [[noreturn]] void ProgramMain(const OsClientStartInfo *StartInfo) {
    int result = CrtEntry(0, nullptr);

    OsProcessHandle ThisProcess = OS_HANDLE_INVALID;
    OsProcessCurrent(&ThisProcess);

    //
    // Terminate the process with the result of main. OsProcessTerminate is noreturn
    // when called with the current process handle.
    //
    OsProcessTerminate(ThisProcess, result);
    __builtin_unreachable();
}
