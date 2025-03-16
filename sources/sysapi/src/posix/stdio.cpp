#include <posix/stdio.h>

#include <posix/errno.h>
#include <posix/stdint.h>
#include <posix/ext/args.h>

#include <bezos/facility/process.h>
#include <bezos/facility/device.h>
#include <bezos/ext/args.h>

#include "private.hpp"

#define STB_SPRINTF_IMPLEMENTATION 1
#include "stb_sprintf.h"

#include <map>

struct OsImplPosixFile {
    int fd;
    int error;
    int eof;
    size_t offset;
};

static constinit OsImplPosixFile kStandardIn = { 0 };
static constinit OsImplPosixFile kStandardOut = { 1 };
static constinit OsImplPosixFile kStandardError = { 2 };

static std::map<int, OsDeviceHandle> &GlobalFdTable() {
    static std::map<int, OsDeviceHandle> sTable;
    return sTable;
};

namespace {
struct InitStandardIo {
    InitStandardIo() {
        OsProcessInfo info{};
        ASSERT_OS_SUCCESS(OsProcessStat(OS_HANDLE_INVALID, &info));

        const OsProcessParam *param = nullptr;
        OsStatus status = OsProcessFindArg(&info, &kPosixInitGuid, &param);
        if (status == OsStatusSuccess) {
            DebugLog(eOsLogDebug, "Initializing standard I/O from process arguments\n");
            const auto *args = reinterpret_cast<const OsPosixInitArgs*>(param->Data);
            auto& fds = GlobalFdTable();
            fds[0] = args->StandardIn;
            fds[1] = args->StandardOut;
            fds[2] = args->StandardError;
        } else {
            Unimplemented();
        }
    }

    ~InitStandardIo() {

    }
};

static const InitStandardIo kInitIo{};
}

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

int feof(FILE *file) noexcept {
    return file->eof;
}

int ferror(FILE *file) noexcept {
    return file->error;
}

int fileno(FILE *file) noexcept {
    return file->fd;
}

size_t fread(void *data, size_t size, size_t n, FILE *file) noexcept {
    if (size == 0 || n == 0) {
        return 0;
    }

    OsDeviceReadRequest request {
        .BufferFront = (char*)data,
        .BufferBack = (char*)data + size * n,
        .Offset = file->offset,
        .Timeout = OS_TIMEOUT_INFINITE,
    };
    OsSize read = 0;
    OsStatus status = OsDeviceRead(GlobalFdTable()[file->fd], request, &read);
    if (status != OsStatusSuccess) {
        file->error = status;
        return 0;
    }

    file->offset += read;
    return read / size;
}

size_t fwrite(const void *data, size_t size, size_t n, FILE *file) noexcept {
    if (size == 0 || n == 0) {
        return 0;
    }

    OsDeviceWriteRequest request {
        .BufferFront = (char*)data,
        .BufferBack = (char*)data + size * n,
        .Offset = file->offset,
        .Timeout = OS_TIMEOUT_INFINITE,
    };
    OsSize written = 0;
    OsStatus status = OsDeviceWrite(GlobalFdTable()[file->fd], request, &written);
    if (status != OsStatusSuccess) {
        file->error = status;
        return 0;
    }

    file->offset += written;
    return written / size;
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
