#include <posix/stdlib.h>

#include <posix/errno.h>
#include <posix/assert.h>

#include <bezos/facility/process.h>

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
    errno = ENOSYS;
    return 0.0;
}

int atoi(const char *) noexcept {
    errno = ENOSYS;
    return 0;
}

long int atol(const char *) noexcept {
    errno = ENOSYS;
    return 0;
}

long long int atoll(const char *) noexcept {
    errno = ENOSYS;
    return 0;
}

double strtod(const char *, char **) noexcept {
    errno = ENOSYS;
    return 0.0;
}

float strtof(const char *, char **) noexcept {
    errno = ENOSYS;
    return 0.0f;
}

long strtol(const char *, char **, int) noexcept {
    errno = ENOSYS;
    return 0;
}

long long strtoll(const char *, char **, int) noexcept {
    errno = ENOSYS;
    return 0;
}

unsigned long strtoul(const char *, char **, int) noexcept {
    errno = ENOSYS;
    return 0;
}

unsigned long long strtoull(const char *, char **, int) noexcept {
    errno = ENOSYS;
    return 0;
}

void *malloc(size_t) noexcept {
    errno = ENOMEM;
    return nullptr;
}

void *calloc(size_t, size_t) noexcept {
    errno = ENOMEM;
    return nullptr;
}

void *realloc(void *, size_t) noexcept {
    errno = ENOMEM;
    return nullptr;
}

void free(void *) noexcept {
    //
    // Empty for now
    //
}

void qsort(void *, size_t, size_t, int (*)(const void *, const void *)) {
    errno = ENOSYS;
}

char *getenv(const char *) noexcept {
    errno = ENOSYS;
    return nullptr;
}

extern int putenv(char *) noexcept {
    errno = ENOSYS;
    return -1;
}

extern int setenv(const char *, const char *, int) noexcept {
    errno = ENOSYS;
    return -1;
}

extern int unsetenv(const char *) noexcept {
    errno = ENOSYS;
    return -1;
}

void srand(unsigned) noexcept {
    errno = ENOSYS;
}

int rand(void) noexcept {
    errno = ENOSYS;
    return 0;
}

int rand_r(unsigned *) noexcept {
    errno = ENOSYS;
    return 0;
}

char *mkdtemp(char *) noexcept {
    errno = ENOSYS;
    return nullptr;
}

int mkstemp(char *) noexcept {
    errno = ENOSYS;
    return -1;
}

char *mktemp(char *) noexcept {
    errno = ENOSYS;
    return nullptr;
}
