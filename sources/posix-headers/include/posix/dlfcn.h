#ifndef BEZOS_POSIX_DLFCN_H
#define BEZOS_POSIX_DLFCN_H 1

#include <detail/cxx.h>

BZP_API_BEGIN

#define RTLD_LAZY 0x0
#define RTLD_NOW 0x1

#define RTLD_GLOBAL (1 << 1)
#define RTLD_LOCAL (1 << 2)

extern int dlclose(void *);
extern char *dlerror(void);
extern void *dlopen(const char *, int);
extern void *dlsym(void *, const char *);

BZP_API_END

#endif /* BEZOS_POSIX_DLFCN_H */
