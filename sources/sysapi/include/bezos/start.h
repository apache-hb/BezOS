#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

struct OsClientStartInfo {
    /// @brief Base address of the tls data
    const void *TlsInit;

    /// @brief Size of the tls data init area
    size_t TlsDataSize;

    /// @brief Size of the tls bss area
    size_t TlsBssSize;
};

#ifdef __cplusplus
}
#endif
