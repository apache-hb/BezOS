#ifndef BEZOS_POSIX_STRING_H
#define BEZOS_POSIX_STRING_H 1

#include <detail/cxx.h>
#include <detail/attributes.h>

#include <stddef.h>

/*
 * strcasecmp and strncasecmp in BSD4.4 originally appeared in string.h
 * maintain compatibility when _DEFAULT_SOURCE or _BSD_SOURCE is defined.
 * TODO: for now this is always enabled due to zsh not correctly defining
 *       _DEFAULT_SOURCE or _BSD_SOURCE.
 */
#if defined(_DEFAULT_SOURCE) || defined(_BSD_SOURCE) || 1
#   include <strings.h>
#endif /* _DEFAULT_SOURCE || _BSD_SOURCE */

BZP_API_BEGIN

__attribute__((__nothrow__, __nonblocking__, __nonnull__, __returns_nonnull__))
extern void *memcpy(void *__BZ_RESTRICT, const void *__BZ_RESTRICT, size_t);

__attribute__((__nothrow__, __nonblocking__, __nonnull__, __returns_nonnull__))
extern void *memmove(void *, const void *, size_t);

__attribute__((__nothrow__, __nonblocking__, __nonnull__, __returns_nonnull__))
extern void *memset(void *, int __c, size_t __n);

__attribute__((__nothrow__, __nonblocking__, __nonnull__))
extern int memcmp(const void *, const void *, size_t);

__attribute__((__nothrow__, __nonblocking__, __nonnull__))
extern void *memchr(const void *, int __c, size_t);

__attribute__((__nothrow__, __nonblocking__, __nonnull__))
extern size_t strlen(const char *);

__attribute__((__nothrow__, __nonblocking__, __nonnull__))
extern size_t strnlen(const char *, size_t);

__attribute__((__nothrow__, __nonblocking__, __nonnull__))
extern int strcmp(const char *, const char *);

__attribute__((__nothrow__, __nonblocking__, __nonnull__))
extern int strncmp(const char *, const char *, size_t);

extern char *strcat(char *, const char *) BZP_NONNULL_ALL;

extern char *strncat(char *, const char *, size_t) BZP_NONNULL_ALL;

extern char *strchr(const char *, int) BZP_NONNULL_ALL;

extern char *strrchr(const char *, int) BZP_NONNULL_ALL;

extern char *strstr(const char *, const char *) BZP_NONNULL_ALL;

extern char *strpbrk(const char *, const char *) BZP_NONNULL_ALL;

extern size_t strspn(const char *, const char *) BZP_NONNULL_ALL;

extern size_t strcspn(const char *, const char *) BZP_NONNULL_ALL;

extern char *strtok(char *, const char *) BZP_NONNULL_ALL;

extern int strcoll(const char *, const char *) BZP_NONNULL_ALL;

extern size_t strxfrm(char *, const char *, size_t) BZP_NONNULL_ALL;

extern char *strcpy(char *, const char *) BZP_NONNULL_ALL;

extern char *strncpy(char *, const char *, size_t) BZP_NONNULL_ALL;

__attribute__((__nothrow__, __nonnull__, __malloc__, __allocating__))
extern char *strdup(const char *);

__attribute__((__nothrow__, __nonnull__, __malloc__, __allocating__))
extern char *strndup(const char *, size_t);

extern char *strerror(int);

extern char *strsignal(int);

BZP_API_END

#endif /* BEZOS_POSIX_STRING_H */
