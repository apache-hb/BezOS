#ifndef BEZOS_POSIX_STRING_H
#define BEZOS_POSIX_STRING_H 1

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void *memcpy(void *, const void *, size_t);

extern void *memmove(void *, const void *, size_t);

extern void *memset(void *, int, size_t);

extern int memcmp(const void *, const void *, size_t);

extern void *memchr(const void *, int, size_t);

extern size_t strlen(const char *);

extern int strcmp(const char *, const char *);

extern int strncmp(const char *, const char *, size_t);

extern char *strcat(char *, const char *);

extern char *strncat(char *, const char *, size_t);

extern char *strchr(const char *, int);

extern char *strrchr(const char *, int);

extern char *strstr(const char *, const char *);

extern char *strpbrk(const char *, const char *);

extern size_t strspn(const char *, const char *);

extern size_t strcspn(const char *, const char *);

extern char *strtok(char *, const char *);

extern int strcoll(const char *, const char *);

extern size_t strxfrm(char *, const char *, size_t);

extern char *strcpy(char *, const char *);

extern char *strncpy(char *, const char *, size_t);

extern char *strdup(const char *);

extern char *strndup(const char *, size_t);

extern char *strerror(int);

extern char *strsignal(int);

#ifdef __cplusplus
}
#endif

#endif /* BEZOS_POSIX_STRING_H */
