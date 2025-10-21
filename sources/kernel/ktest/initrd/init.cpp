#include <bezos/bezos.h>

#include <utility>

OS_EXTERN OS_NORETURN
[[gnu::force_align_arg_pointer]]
void ClientStart(const struct OsClientStartInfo *) {
    // OsProcessTerminate(OS_HANDLE_INVALID, 0);
    std::unreachable();
}
