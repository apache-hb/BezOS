#ifndef BEZOS_POSIX_SETJMP_H
#define BEZOS_POSIX_SETJMP_H 1

#ifdef __cplusplus
extern "C" {
#endif

typedef struct OsImplPosixJmpBuf {
    long long r[8];
} jmp_buf[1];

extern int setjmp(jmp_buf);

[[noreturn]]
extern void longjmp(jmp_buf, int);

#ifdef __cplusplus
}
#endif

#endif /* BEZOS_POSIX_SETJMP_H */
