#ifndef BEZOS_POSIX_CTYPE_H
#define BEZOS_POSIX_CTYPE_H 1

#ifdef __cplusplus
extern "C" {
#endif

extern int isprint(int);

extern int isalpha(int);

extern int isalnum(int);

extern int isdigit(int);

extern int isspace(int);

extern int isupper(int);
extern int islower(int);

extern int toupper(int);
extern int tolower(int);

#ifdef __cplusplus
}
#endif

#endif /* BEZOS_POSIX_CTYPE_H */
