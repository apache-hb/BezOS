#ifndef BEZOS_POSIX_STRINGS_H
#define BEZOS_POSIX_STRINGS_H 1

#include <detail/size.h>
#include <detail/cxx.h>
#include <detail/attributes.h>

BZP_API_BEGIN

extern int strcasecmp(const char *, const char *) BZP_NONNULL_ALL;

extern int strncasecmp(const char *, const char *, size_t) BZP_NONNULL_ALL;

BZP_API_END

#endif /* BEZOS_POSIX_STRINGS_H */
