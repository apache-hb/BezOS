#ifndef BEZOS_POSIX_STDIO_H
#define BEZOS_POSIX_STDIO_H 1

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct OsImplPosixFile;

typedef struct OsImplPosixFile FILE;

#define EOF (-1)

extern FILE *OsImplPosixStandardIn(void);
extern FILE *OsImplPosixStandardOut(void);
extern FILE *OsImplPosixStandardError(void);

#define stdin (OsImplPosixStandardIn())
#define stdout (OsImplPosixStandardOut())
#define stderr (OsImplPosixStandardError())

extern int fclose(FILE *);
extern FILE *fopen(const char *, const char *);

extern int fflush(FILE *);

extern int fprintf(FILE *, const char *, ...);

extern int printf(const char *, ...);

extern int sprintf(char *, const char *, ...);

extern size_t fread(void *, size_t, size_t, FILE *);

extern size_t fwrite(const void *, size_t, size_t, FILE *);

extern int fseek(FILE *, long int, int);

extern long int ftell(FILE *);

extern int feof(FILE *);

extern int ferror(FILE *);

extern int remove(const char *);

extern void perror(const char *);

#ifdef __cplusplus
}
#endif

#endif /* BEZOS_POSIX_STDIO_H */
