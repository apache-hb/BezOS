#ifndef BEZOS_POSIX_SETJMP_H
#define BEZOS_POSIX_SETJMP_H 1

#include <detail/cxx.h>

BZP_API_BEGIN

typedef struct OsImplPosixJmpBuf {
    long long r[8];
} jmp_buf[1];

typedef struct OsImplPosixSigJmpBuf {
    long long r[8];
    int mask_was_saved;
} sigjmp_buf[1];

extern int setjmp(jmp_buf) BZP_NOEXCEPT;

[[noreturn]]
extern void longjmp(jmp_buf, int) BZP_NOEXCEPT;

extern int sigsetjmp(sigjmp_buf, int) BZP_NOEXCEPT;

[[noreturn]]
extern void siglongjmp(sigjmp_buf, int) BZP_NOEXCEPT;

BZP_API_END

#endif /* BEZOS_POSIX_SETJMP_H */
