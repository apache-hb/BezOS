#ifndef BEZOS_POSIX_CTYPE_H
#define BEZOS_POSIX_CTYPE_H 1

#include <detail/cxx.h>

BZP_API_BEGIN

extern int isprint(int) BZP_NOEXCEPT;

extern int isalpha(int) BZP_NOEXCEPT;

extern int isalnum(int) BZP_NOEXCEPT;

extern int isdigit(int) BZP_NOEXCEPT;

extern int isspace(int) BZP_NOEXCEPT;

extern int ispunct(int) BZP_NOEXCEPT;

extern int iscntrl(int) BZP_NOEXCEPT;

extern int isgraph(int) BZP_NOEXCEPT;

extern int isupper(int) BZP_NOEXCEPT;
extern int islower(int) BZP_NOEXCEPT;

extern int toupper(int) BZP_NOEXCEPT;
extern int tolower(int) BZP_NOEXCEPT;

BZP_API_END

#endif /* BEZOS_POSIX_CTYPE_H */
