#ifndef BEZOS_POSIX_CTYPE_H
#define BEZOS_POSIX_CTYPE_H 1

#include <detail/cxx.h>

BEZOS_POSIX_API_BEGIN

extern int isprint(int) BEZOS_POSIX_NOEXCEPT;

extern int isalpha(int) BEZOS_POSIX_NOEXCEPT;

extern int isalnum(int) BEZOS_POSIX_NOEXCEPT;

extern int isdigit(int) BEZOS_POSIX_NOEXCEPT;

extern int isspace(int) BEZOS_POSIX_NOEXCEPT;

extern int ispunct(int) BEZOS_POSIX_NOEXCEPT;

extern int isupper(int) BEZOS_POSIX_NOEXCEPT;
extern int islower(int) BEZOS_POSIX_NOEXCEPT;

extern int toupper(int) BEZOS_POSIX_NOEXCEPT;
extern int tolower(int) BEZOS_POSIX_NOEXCEPT;

BEZOS_POSIX_API_END

#endif /* BEZOS_POSIX_CTYPE_H */
