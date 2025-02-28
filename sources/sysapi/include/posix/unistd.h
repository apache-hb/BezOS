#ifndef BEZOS_POSIX_UNISTD_H
#define BEZOS_POSIX_UNISTD_H 1

#include <detail/id.h>
#include <detail/size.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

extern ssize_t read(int, void *, size_t);

extern ssize_t write(int, const void *, size_t);

#ifdef __cplusplus
}
#endif

#endif /* BEZOS_POSIX_UNISTD_H */
