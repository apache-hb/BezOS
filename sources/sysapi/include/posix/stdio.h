#ifndef BEZOS_POSIX_STDIO_H
#define BEZOS_POSIX_STDIO_H 1

#include <detail/cxx.h>

#include <stddef.h>
#include <stdarg.h>

BZP_API_BEGIN

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

extern int fclose(FILE *) BZP_NOEXCEPT;

extern FILE *fdopen(int, const char *) BZP_NOEXCEPT;

extern FILE *fopen(const char *, const char *) BZP_NOEXCEPT;

extern void clearerr(FILE *) BZP_NOEXCEPT;

extern int fflush(FILE *) BZP_NOEXCEPT;

extern int fileno(FILE *) BZP_NOEXCEPT;

extern int fputs(const char *, FILE *) BZP_NOEXCEPT;

extern int fprintf(FILE *, const char *, ...) BZP_NOEXCEPT;

extern int printf(const char *, ...) BZP_NOEXCEPT;

extern int sprintf(char *, const char *, ...) BZP_NOEXCEPT;

extern int snprintf(char *, size_t, const char *, ...) BZP_NOEXCEPT;

extern int vsprintf(char *, const char *, va_list) BZP_NOEXCEPT;

extern int vsnprintf(char *, size_t, const char *, va_list) BZP_NOEXCEPT;

extern int vprintf(const char *, va_list) BZP_NOEXCEPT;

extern int vfprintf(FILE *, const char *, va_list) BZP_NOEXCEPT;

extern size_t fread(void *, size_t, size_t, FILE *) BZP_NOEXCEPT;

extern size_t fwrite(const void *, size_t, size_t, FILE *) BZP_NOEXCEPT;

extern int fseek(FILE *, long int, int) BZP_NOEXCEPT;

extern long int ftell(FILE *) BZP_NOEXCEPT;

extern int feof(FILE *) BZP_NOEXCEPT;

extern int ferror(FILE *) BZP_NOEXCEPT;

extern int remove(const char *) BZP_NOEXCEPT;

extern void perror(const char *) BZP_NOEXCEPT;

extern int puts(const char*) BZP_NOEXCEPT;

extern int putc(int, FILE *) BZP_NOEXCEPT;

extern char *fgets(char *, int, FILE *) BZP_NOEXCEPT;

extern int putchar(int) BZP_NOEXCEPT;

BZP_API_END

#endif /* BEZOS_POSIX_STDIO_H */
