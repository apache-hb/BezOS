#ifndef BEZOS_POSIX_CTYPE_H
#define BEZOS_POSIX_CTYPE_H 1

#include <detail/cxx.h>

BZP_API_BEGIN

extern int isascii(int);

extern int isprint(int);

extern int isalpha(int);

extern int isalnum(int);

extern int isdigit(int);

extern int isxdigit(int);

extern int isspace(int);

extern int ispunct(int);

extern int iscntrl(int);

extern int isgraph(int);

extern int isupper(int);
extern int islower(int);

extern int toupper(int);
extern int tolower(int);

BZP_API_END

#endif /* BEZOS_POSIX_CTYPE_H */
