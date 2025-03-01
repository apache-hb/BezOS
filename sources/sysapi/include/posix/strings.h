#ifndef BEZOS_POSIX_STRINGS_H
#define BEZOS_POSIX_STRINGS_H 1

#include <detail/size.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int strcasecmp(const char *, const char *);

extern int strncasecmp(const char *, const char *, size_t);

#ifdef __cplusplus
}
#endif

#endif /* BEZOS_POSIX_STRINGS_H */
