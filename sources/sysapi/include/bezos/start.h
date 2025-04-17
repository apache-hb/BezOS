#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

struct OsClientStartInfo {
    const void *TlsInit;
    size_t TlsInitSize;
};

#ifdef __cplusplus
}
#endif
