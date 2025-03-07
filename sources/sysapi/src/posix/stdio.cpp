#include <posix/stdio.h>

#include <posix/errno.h>
#include <posix/stdint.h>

struct OsImplPosixFile {
    int fd;
};

static constinit OsImplPosixFile kStandardIn = { 0 };
static constinit OsImplPosixFile kStandardOut = { 1 };
static constinit OsImplPosixFile kStandardError = { 2 };

FILE *OsImplPosixStandardIn(void) noexcept {
    return &kStandardIn;
}

FILE *OsImplPosixStandardOut(void) noexcept {
    return &kStandardOut;
}

FILE *OsImplPosixStandardError(void) noexcept {
    return &kStandardError;
}

int fclose(FILE *) noexcept {
    errno = ENOSYS;
    return -1;
}

FILE *fopen(const char *, const char *) noexcept {
    errno = ENOSYS;
    return nullptr;
}

int fprintf(FILE *, const char *, ...) noexcept {
    errno = ENOSYS;
    return -1;
}

int printf(const char *format, ...) noexcept {
    va_list args;
    va_start(args, format);
    int result = vprintf(format, args);
    va_end(args);
    return result;
}

int sprintf(char *dst, const char *format, ...) noexcept {
    va_list args;
    va_start(args, format);
    int result = vsprintf(dst, format, args);
    va_end(args);
    return result;
}

int snprintf(char *dst, size_t size, const char *format, ...) noexcept {
    va_list args;
    va_start(args, format);
    int result = vsnprintf(dst, size, format, args);
    va_end(args);
    return result;
}

int vsprintf(char *dst, const char *format, va_list args) noexcept {
    return vsnprintf(dst, SIZE_MAX, format, args);
}

int vsnprintf(char *, size_t, const char *, va_list) noexcept {
    errno = ENOSYS;
    return -1;
}

int vprintf(const char *format, va_list args) noexcept {
    return vfprintf(stdout, format, args);
}

int vfprintf(FILE *, const char *, va_list) noexcept {
    errno = ENOSYS;
    return -1;
}
