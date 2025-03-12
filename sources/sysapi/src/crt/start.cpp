#include <bezos/facility/process.h>

#include <bezos/start.h>

extern "C" void (*__init_array_start[])(void) __attribute__((weak));
extern "C" void (*__init_array_end[])(void) __attribute__((weak));

extern "C" void (*__fini_array_start[])(void) __attribute__((weak));
extern "C" void (*__fini_array_end[])(void) __attribute__((weak));

extern "C" int CrtEntry(int argc, const char *const *argv) asm("main");

static const char *const kDefaultArgs[] = { "program.elf", nullptr };

static void CallInitArray() {
    if (__init_array_start == nullptr) {
        return;
    }

    for (void (**fn)() = __init_array_start; fn < __init_array_end; ++fn) {
        (*fn)();
    }
}

static void CallFiniArray() {
    if (__fini_array_start == nullptr) {
        return;
    }

    for (void (**fn)() = __fini_array_start; fn < __fini_array_end; ++fn) {
        (*fn)();
    }
}

extern "C" [[noreturn]] void ProgramMain(const OsClientStartInfo *StartInfo) {
    CallInitArray();

    int result = CrtEntry(1, kDefaultArgs);

    CallFiniArray();

    OsProcessHandle ThisProcess = OS_HANDLE_INVALID;
    OsProcessCurrent(&ThisProcess);

    //
    // Terminate the process with the result of main. OsProcessTerminate is noreturn
    // when called with the current process handle.
    //
    OsProcessTerminate(ThisProcess, result);
    __builtin_unreachable();
}
