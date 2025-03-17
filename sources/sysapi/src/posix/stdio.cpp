#include <posix/stdio.h>

#include <posix/errno.h>
#include <posix/stdint.h>
#include <posix/ext/args.h>

#include <bezos/facility/process.h>
#include <bezos/facility/device.h>
#include <bezos/ext/args.h>

#include "bezos/handle.h"
#include "private.hpp"

#include <atomic>

#define STB_SPRINTF_IMPLEMENTATION 1
#include "stb_sprintf.h"

struct OsImplPosixFile {
    int fd;
    int error;
    int eof;
    size_t offset;
};

namespace {

struct OsPosixFd {
    OsDeviceHandle handle;
    size_t offset;
};

// TODO: need an actual map
OsPosixFd gFdMap[32];
// std::atomic<int> gFdCounter{3};

class InitStandardIo {
    OsImplPosixFile mStandardIn = { 0 };
    OsImplPosixFile mStandardOut = { 1 };
    OsImplPosixFile mStandardError = { 2 };

public:
    InitStandardIo() {
        OsProcessInfo info{};
        ASSERT_OS_SUCCESS(OsProcessStat(OS_HANDLE_INVALID, &info));

        const OsProcessParam *param = nullptr;
        OsStatus status = OsProcessFindArg(&info, &kPosixInitGuid, &param);
        if (status == OsStatusSuccess) {
            const OsPosixInitArgs *args = reinterpret_cast<const OsPosixInitArgs*>(param->Data);
            gFdMap[0] = { args->StandardIn };
            gFdMap[1] = { args->StandardOut };
            gFdMap[2] = { args->StandardError };
        } else {
            Unimplemented();
        }
    }

    ~InitStandardIo() {

    }

    OsImplPosixFile *in() noexcept {
        return &mStandardIn;
    }

    OsImplPosixFile *out() noexcept {
        return &mStandardOut;
    }

    OsImplPosixFile *error() noexcept {
        return &mStandardError;
    }
};

InitStandardIo& GlobalStandardIo() {
    static InitStandardIo sStandardIo;
    return sStandardIo;
}
}

FILE *OsImplPosixStandardIn(void) noexcept {
    return GlobalStandardIo().in();
}

FILE *OsImplPosixStandardOut(void) noexcept {
    return GlobalStandardIo().out();
}

FILE *OsImplPosixStandardError(void) noexcept {
    return GlobalStandardIo().error();
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

int feof(FILE *file) noexcept {
    return file->eof;
}

int ferror(FILE *file) noexcept {
    return file->error;
}

int fileno(FILE *file) noexcept {
    return file->fd;
}

size_t fread(void *dst, size_t size, size_t count, FILE *file) noexcept {
    if (size == 0 || count == 0) {
        return 0;
    }

    OsPosixFd *fd = &gFdMap[fileno(file)];
    OsDeviceReadRequest request {
        .BufferFront = static_cast<char*>(dst),
        .BufferBack = static_cast<char*>(dst) + size * count,
        .Offset = fd->offset,
        .Timeout = OS_TIMEOUT_INFINITE,
    };
    OsSize result = 0;

    OsStatus status = OsDeviceRead(fd->handle, request, &result);
    if (status != OsStatusSuccess) {
        file->error = status;
        return -1;
    }

    if (result != size * count) {
        file->eof = 1;
    }

    fd->offset += result;
    return result / size;
}

size_t fwrite(const void *src, size_t size, size_t count, FILE *file) noexcept {
    if (size == 0 || count == 0) {
        return 0;
    }

    OsPosixFd *fd = &gFdMap[fileno(file)];
    OsDeviceWriteRequest request {
        .BufferFront = static_cast<const char*>(src),
        .BufferBack = static_cast<const char*>(src) + size * count,
        .Offset = fd->offset,
        .Timeout = OS_TIMEOUT_INFINITE,
    };
    OsSize result = 0;

    OsStatus status = OsDeviceWrite(fd->handle, request, &result);
    if (status != OsStatusSuccess) {
        file->error = status;
        return -1;
    }

    if (result != size * count) {
        file->eof = 1;
    }

    fd->offset += result;
    return result / size;
}

int fprintf(FILE *file, const char *format, ...) noexcept {
    va_list args;
    va_start(args, format);
    int result = vfprintf(file, format, args);
    va_end(args);
    return result;
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
    int result = STB_SPRINTF_DECORATE(vsprintf)(dst, format, args);
    va_end(args);
    return result;
}

int snprintf(char *dst, size_t size, const char *format, ...) noexcept {
    va_list args;
    va_start(args, format);
    int result = STB_SPRINTF_DECORATE(vsnprintf)(dst, size, format, args);
    va_end(args);
    return result;
}

int vsprintf(char *dst, const char *format, va_list args) noexcept {
    return STB_SPRINTF_DECORATE(vsprintf)(dst, format, args);
}

int vsnprintf(char *dst, size_t size, const char *format, va_list args) noexcept {
    return STB_SPRINTF_DECORATE(vsnprintf)(dst, size, format, args);
}

int vprintf(const char *format, va_list args) noexcept {
    return vfprintf(stdout, format, args);
}

struct UserPrintBuffer {
    char *buffer;
    FILE *file;
};

static char *OsPrintCallback(const char *buffer, void *user, int length) {
    UserPrintBuffer *data = static_cast<UserPrintBuffer*>(user);
    fwrite(buffer, 1, length, data->file);
    return data->buffer;
}

int vfprintf(FILE *file, const char *format, va_list args) noexcept {
    char buffer[STB_SPRINTF_MIN]{};

    UserPrintBuffer user { buffer, file };
    return STB_SPRINTF_DECORATE(vsprintfcb)(OsPrintCallback, &user, buffer, format, args);
}

int puts(const char *str) noexcept {
    int status = printf("%s\n", str);
    if (status < 0) {
        return EOF;
    }

    return status;
}

int putc(int c, FILE *file) noexcept {
    return fputc(c, file);
}

int fputc(int c, FILE *file) noexcept {
    char buffer[1] = { static_cast<char>(c) };
    int status = fwrite(buffer, 1, 1, file);
    if (status != 1) {
        return EOF;
    }

    return c;
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
