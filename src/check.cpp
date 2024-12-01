#include "kernel.h"

#include "arch.h"
#include "crt.h"

#include <stdint.h>
#include <limits.h>

[[noreturn]]
void KmHalt(void) {
    for (;;) {
        __cli();
        __halt();
    }
}

struct FormatResult {
    char buffer[64 + 1];
};

// base10
template<typename T>
static FormatResult FormatInt10(T value) {
    FormatResult result = {};
    char *ptr = result.buffer;

    if (value < 0) {
        *ptr++ = '-';
        value = -value;
    }

    char buffer[64];
    char *bufptr = buffer;
    do {
        *bufptr++ = '0' + (value % 10);
        value /= 10;
    } while (value);

    while (bufptr != buffer) {
        *ptr++ = *--bufptr;
    }

    *ptr = '\0';

    return result;
}

void KmBugCheck(const char *message, const char *file, unsigned line) {
    // TODO: string formatting of some sort
    KmDebugMessage("[BUGCHECK]: ");
    KmDebugMessage(message);
    KmDebugMessage("\n");
    KmDebugMessage("File: ");
    KmDebugMessage(file);
    KmDebugMessage(":");
    FormatResult linet = FormatInt10(line);
    KmDebugMessage(linet.buffer);
    KmDebugMessage("\n");

    KmHalt();
}
