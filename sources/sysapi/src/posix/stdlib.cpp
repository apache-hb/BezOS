#include <posix/stdlib.h>

#include <posix/errno.h>
#include <posix/assert.h>
#include <posix/unistd.h>
#include <posix/string.h>

#include <bezos/facility/process.h>

#include <rpmalloc/rpmalloc.h>

#include "private.hpp"

namespace {
struct InitMalloc {
    InitMalloc() {
        rpmalloc_initialize();
    }

    ~InitMalloc() {
        rpmalloc_finalize();
    }

    void *malloc(size_t size) {
        return rpmalloc(size);
    }

    void *aligned_alloc(size_t alignment, size_t size) {
        return rpaligned_alloc(alignment, size);
    }

    void *calloc(size_t n, size_t size) {
        return rpcalloc(n, size);
    }

    void *realloc(void *ptr, size_t size) {
        return rprealloc(ptr, size);
    }

    void free(void *ptr) {
        rpfree(ptr);
    }
};

InitMalloc& GlobalMalloc() {
    static InitMalloc sMalloc;
    return sMalloc;
}
}

void abort(void) noexcept {
    exit(EXIT_FAILURE);
}

void exit(int exitcode) noexcept {
    OsProcessHandle handle = OS_HANDLE_INVALID;
    OsStatus status = OsProcessCurrent(&handle);
    assert(status == OsStatusSuccess);

    //
    // OsProcessTerminate when called with the current process handle
    // will terminate the current process and never return.
    //
    status = OsProcessTerminate(handle, exitcode);
    __builtin_unreachable();
}

int abs(int v) noexcept {
    return v < 0 ? -v : v;
}

long labs(long v) noexcept {
    return v < 0 ? -v : v;
}

long long llabs(long long v) noexcept {
    return v < 0 ? -v : v;
}

double atof(const char *) noexcept {
    Unimplemented();
    return 0.0;
}

int atoi(const char *) noexcept {
    Unimplemented();
    return 0;
}

long int atol(const char *) noexcept {
    Unimplemented();
    return 0;
}

long long int atoll(const char *) noexcept {
    Unimplemented();
    return 0;
}

double strtod(const char *, char **) noexcept {
    Unimplemented();
    return 0.0;
}

float strtof(const char *, char **) noexcept {
    Unimplemented();
    return 0.0f;
}

long strtol(const char *, char **, int) noexcept {
    Unimplemented();
    return 0;
}

long long strtoll(const char *, char **, int) noexcept {
    Unimplemented();
    return 0;
}

unsigned long strtoul(const char *, char **, int) noexcept {
    Unimplemented();
    return 0;
}

unsigned long long strtoull(const char *, char **, int) noexcept {
    Unimplemented();
    return 0;
}

long double strtold(const char *, char **) noexcept {
    Unimplemented();
    return 0.0;
}

void *malloc(size_t size) noexcept {
    return GlobalMalloc().malloc(size);
}

void *aligned_alloc(size_t alignment, size_t size) noexcept {
    return GlobalMalloc().aligned_alloc(alignment, size);
}

void *calloc(size_t n, size_t size) noexcept {
    return GlobalMalloc().calloc(n, size);
}

void *realloc(void *ptr, size_t size) noexcept {
    return GlobalMalloc().realloc(ptr, size);
}

void free(void *ptr) noexcept {
    return GlobalMalloc().free(ptr);
}

void qsort(void *, size_t, size_t, int (*)(const void *, const void *)) {
    Unimplemented();
}

char *getenv(const char *name) noexcept {
    Unimplemented();
    DebugLog(eOsLogInfo, "POSIX getenv: '%s'", name);

    char **iter = environ;
    size_t len = strlen(name);
    while (*iter) {
        if (strncmp(*iter, name, len) == 0) {
            return const_cast<char*>(*iter + len + 1);
        }

        iter += 1;
    }

    errno = ENOENT;
    return nullptr;
}

extern int putenv(char *) noexcept {
    Unimplemented();
    return -1;
}

extern int setenv(const char *name, const char *val, int overwrite) noexcept {
    Unimplemented();
    DebugLog(eOsLogInfo, "POSIX setenv: '%s'='%s'%s", name, val, overwrite ? " (overwrite)" : "");
    return -1;
}

extern int unsetenv(const char *) noexcept {
    Unimplemented();
    return -1;
}

void srand(unsigned) noexcept {
    Unimplemented();
}

int rand(void) noexcept {
    Unimplemented();
    return 0;
}

int rand_r(unsigned *) noexcept {
    Unimplemented();
    return 0;
}

char *mkdtemp(char *) noexcept {
    Unimplemented();
    return nullptr;
}

int mkstemp(char *) noexcept {
    Unimplemented();
    return -1;
}

char *mktemp(char *) noexcept {
    Unimplemented();
    return nullptr;
}

int atexit(void (*)(void)) noexcept {
    Unimplemented();
    return -1;
}

extern "C" int __cxa_atexit(void (*)(void *), void *, void *) noexcept {
    Unimplemented();
    return 0;
}

extern "C" void *__dso_handle = nullptr;
