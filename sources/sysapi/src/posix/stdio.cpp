#include <posix/stdio.h>

#include <posix/errno.h>
#include <posix/stdint.h>

#include "private.hpp"

struct OsImplPosixFile {
    int fd;
};

static constinit OsImplPosixFile kStandardIn = { 0 };
static constinit OsImplPosixFile kStandardOut = { 1 };
static constinit OsImplPosixFile kStandardError = { 2 };

FILE *OsImplPosixStandardIn(void) noexcept {
    Unimplemented();
    return &kStandardIn;
}

FILE *OsImplPosixStandardOut(void) noexcept {
    Unimplemented();
    return &kStandardOut;
}

FILE *OsImplPosixStandardError(void) noexcept {
    Unimplemented();
    return &kStandardError;
}

int fclose(FILE *) noexcept {
    Unimplemented();
    return -1;
}

FILE *fopen(const char *, const char *) noexcept {
    Unimplemented();
    return nullptr;
}

void clearerr(FILE *) noexcept {
    Unimplemented();
}

FILE *fdopen(int, const char *) noexcept {
    Unimplemented();
    return nullptr;
}

int fseek(FILE *, long, int) noexcept {
    Unimplemented();
    return -1;
}

long ftell(FILE *) noexcept {
    Unimplemented();
    return -1;
}

int fflush(FILE *) noexcept {
    Unimplemented();
    return -1;
}

int feof(FILE *) noexcept {
    Unimplemented();
    return 0;
}

int ferror(FILE *) noexcept {
    Unimplemented();
    return 0;
}

int fileno(FILE *file) noexcept {
    return file->fd;
}

size_t fread(void *, size_t, size_t, FILE *) noexcept {
    Unimplemented();
    return 0;
}

size_t fwrite(const void *, size_t, size_t, FILE *) noexcept {
    Unimplemented();
    return 0;
}

int fprintf(FILE *, const char *, ...) noexcept {
    Unimplemented();
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
    Unimplemented();
    return -1;
}

int vprintf(const char *format, va_list args) noexcept {
    return vfprintf(stdout, format, args);
}

int vfprintf(FILE *, const char *, va_list) noexcept {
    Unimplemented();
    return -1;
}

int puts(const char*) noexcept {
    Unimplemented();
    return -1;
}

int putc(int, FILE *) noexcept {
    Unimplemented();
    return -1;
}

int fputc(int, FILE *) noexcept {
    Unimplemented();
    return -1;
}

int fgetc(FILE *) noexcept {
    Unimplemented();
    return -1;
}

char *fgets(char *, int, FILE *) noexcept {
    Unimplemented();
    return nullptr;
}

int fputs(const char *, FILE *) noexcept {
    Unimplemented();
    return -1;
}

int putchar(int) noexcept {
    Unimplemented();
    return -1;
}

void setbuffer(FILE *, char *, int) noexcept {
    Unimplemented();
}

void setlinebuf(FILE *) noexcept {
    Unimplemented();
}

char *cuserid(char *) noexcept {
    Unimplemented();
    return nullptr;
}
