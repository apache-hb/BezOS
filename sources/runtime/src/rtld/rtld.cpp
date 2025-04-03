#include "load/elf.hpp"

#include <bezos/facility/process.h>
#include <bezos/start.h>

extern "C" [[noreturn, gnu::force_align_arg_pointer]]
void ClientStart(const OsClientStartInfo *) {
    OsProcessHandle CurrentProcess = OS_HANDLE_INVALID;
    OsProcessCurrent(eOsProcessAccessTerminate, &CurrentProcess);
    OsProcessTerminate(CurrentProcess, OsStatusSuccess);
    __builtin_unreachable();
}
