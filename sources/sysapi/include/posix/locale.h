#ifndef BEZOS_POSIX_LOCALE_H
#define BEZOS_POSIX_LOCALE_H 1

#include <detail/cxx.h>

BZP_API_BEGIN

#define LC_COLLATE 0
#define LC_CTYPE 1
#define LC_MESSAGES 2

extern char *setlocale(int, const char *) BZP_NOEXCEPT;

BZP_API_END

#endif /* BEZOS_POSIX_LOCALE_H */
