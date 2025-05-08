#include <posix/stdio.h>

#include <posix/errno.h>
#include <posix/stdint.h>
#include <posix/ext/args.h>

#include <bezos/facility/process.h>
#include <bezos/facility/device.h>
#include <bezos/facility/handle.h>
#include <bezos/ext/args.h>

#include "bezos/handle.h"
#include "private.hpp"

#define STB_SPRINTF_IMPLEMENTATION 1
#include "stb_sprintf.h"

struct OsImplPosixFile {
    int fd;
};

namespace {

struct OsPosixFd {
    OsDeviceHandle handle;
    size_t offset;
    int error;
    int eof;
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
            ASSERT_OS_SUCCESS(OsHandleOpen(args->StandardIn, eOsDeviceAccessRead | eOsDeviceAccessWait | eOsDeviceAccessStat, &gFdMap[0].handle));
            ASSERT_OS_SUCCESS(OsHandleOpen(args->StandardOut, eOsDeviceAccessWrite | eOsDeviceAccessWait | eOsDeviceAccessStat, &gFdMap[1].handle));
            ASSERT_OS_SUCCESS(OsHandleOpen(args->StandardError, eOsDeviceAccessWrite | eOsDeviceAccessWait | eOsDeviceAccessStat, &gFdMap[2].handle));
        }
    }

    OsImplPosixFile *in() {
        return &mStandardIn;
    }

    OsImplPosixFile *out() {
        return &mStandardOut;
    }

    OsImplPosixFile *error() {
        return &mStandardError;
    }
};

InitStandardIo& GlobalStandardIo() {
    static InitStandardIo sStandardIo;
    return sStandardIo;
}
}

FILE *OsImplPosixStandardIn(void) {
    return GlobalStandardIo().in();
}

FILE *OsImplPosixStandardOut(void) {
    return GlobalStandardIo().out();
}

FILE *OsImplPosixStandardError(void) {
    return GlobalStandardIo().error();
}

int fclose(FILE *) {
    Unimplemented();
    return -1;
}

FILE *fopen(const char *, const char *) {
    Unimplemented();
    return nullptr;
}

void clearerr(FILE *) {
    Unimplemented();
}

FILE *fdopen(int, const char *) {
    Unimplemented();
    return nullptr;
}

int fseek(FILE *, long, int) {
    Unimplemented();
    return -1;
}

long ftell(FILE *) {
    Unimplemented();
    return -1;
}

int fflush(FILE *) {
    Unimplemented();
    return -1;
}

int feof(FILE *file) {
    return gFdMap[file->fd].eof;
}

int ferror(FILE *file) {
    return gFdMap[file->fd].error;
}

int fileno(FILE *file) {
    return file->fd;
}

size_t fread(void *dst, size_t size, size_t count, FILE *file) {
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
        gFdMap[fileno(file)].error = status;
        return -1;
    }

    if (result != size * count) {
        gFdMap[fileno(file)].eof = 1;
    }

    fd->offset += result;
    return result / size;
}

size_t fwrite(const void *src, size_t size, size_t count, FILE *file) {
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
        gFdMap[fileno(file)].error = status;
        return -1;
    }

    if (result != size * count) {
        gFdMap[fileno(file)].eof = 1;
    }

    fd->offset += result;
    return result / size;
}

int fprintf(FILE *file, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int result = vfprintf(file, format, args);
    va_end(args);
    return result;
}

int printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    int result = vprintf(format, args);
    va_end(args);
    return result;
}

int sprintf(char *dst, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int result = STB_SPRINTF_DECORATE(vsprintf)(dst, format, args);
    va_end(args);
    return result;
}

int snprintf(char *dst, size_t size, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int result = STB_SPRINTF_DECORATE(vsnprintf)(dst, size, format, args);
    va_end(args);
    return result;
}

int vsprintf(char *dst, const char *format, va_list args) {
    return STB_SPRINTF_DECORATE(vsprintf)(dst, format, args);
}

int vsnprintf(char *dst, size_t size, const char *format, va_list args) {
    return STB_SPRINTF_DECORATE(vsnprintf)(dst, size, format, args);
}

int vprintf(const char *format, va_list args) {
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

int vfprintf(FILE *file, const char *format, va_list args) {
    char buffer[STB_SPRINTF_MIN]{};

    UserPrintBuffer user { buffer, file };
    return STB_SPRINTF_DECORATE(vsprintfcb)(OsPrintCallback, &user, buffer, format, args);
}

int puts(const char *str) {
    int status = printf("%s\n", str);
    if (status < 0) {
        return EOF;
    }

    return status;
}

int putc(int c, FILE *file) {
    return fputc(c, file);
}

int fputc(int c, FILE *file) {
    char buffer[1] = { static_cast<char>(c) };
    int status = fwrite(buffer, 1, 1, file);
    if (status != 1) {
        return EOF;
    }

    return c;
}

int fgetc(FILE *) {
    Unimplemented();
    return -1;
}

char *fgets(char *, int, FILE *) {
    Unimplemented();
    return nullptr;
}

int fputs(const char *, FILE *) {
    Unimplemented();
    return -1;
}

int putchar(int) {
    Unimplemented();
    return -1;
}

void setbuffer(FILE *, char *, int) {
    Unimplemented();
}

void setlinebuf(FILE *) {
    Unimplemented();
}

char *cuserid(char *) {
    Unimplemented();
    return nullptr;
}
