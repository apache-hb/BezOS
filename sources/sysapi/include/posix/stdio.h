#ifndef BEZOS_POSIX_STDIO_H
#define BEZOS_POSIX_STDIO_H 1

#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

struct OsImplPosixFile;

typedef struct OsImplPosixFile FILE;

#define SEEK_CUR 0x100
#define SEEK_END 0x200
#define SEEK_SET 0x300

#define EOF (-1)

extern FILE *OsImplPosixStandardIn(void);
extern FILE *OsImplPosixStandardOut(void);
extern FILE *OsImplPosixStandardError(void);

#define stdin (OsImplPosixStandardIn())
#define stdout (OsImplPosixStandardOut())
#define stderr (OsImplPosixStandardError())

extern int fclose(FILE *);

extern FILE *fdopen(int, const char *);

extern FILE *fopen(const char *, const char *);

extern void clearerr(FILE *);

extern int fflush(FILE *);

extern int fileno(FILE *);

extern int fputs(const char *, FILE *);

extern int fprintf(FILE *, const char *, ...);

extern int printf(const char *, ...);

extern int sprintf(char *, const char *, ...);

extern int snprintf(char *, size_t, const char *, ...);

extern int vsprintf(char *, const char *, va_list);

extern int vsnprintf(char *, size_t, const char *, va_list);

extern int vprintf(const char *, va_list);

extern int vfprintf(FILE *, const char *, va_list);

extern size_t fread(void *, size_t, size_t, FILE *);

extern size_t fwrite(const void *, size_t, size_t, FILE *);

extern int fseek(FILE *, long int, int);

extern long int ftell(FILE *);

extern int feof(FILE *);

extern int ferror(FILE *);

extern int remove(const char *);

extern void perror(const char *);

extern int puts(const char*);

extern int putc(int, FILE *);

extern char *fgets(char *, int, FILE *);

extern int putchar(int);

#ifdef __cplusplus
}
#endif

#endif /* BEZOS_POSIX_STDIO_H */
