#ifndef BEZOS_POSIX_SYS_MMAN_H
#define BEZOS_POSIX_SYS_MMAN_H 1

#include <detail/size.h>
#include <detail/node.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PROT_NONE 0
#define PROT_READ  (1 << 0)
#define PROT_WRITE (1 << 1)
#define PROT_EXEC  (1 << 2)

#define MAP_FIXED (1 << 0)
#define MAP_SHARED (1 << 1)
#define MAP_PRIVATE (1 << 2)

#define MAP_ANONYMOUS (1 << 3)
#define MAP_UNINITIALIZED (1 << 4)

#define POSIX_MADV_NORMAL 0x1
#define POSIX_MADV_DONTNEED 0x2

#define MAP_FAILED ((void*)-1)

extern void *mmap(void *, size_t, int, int, int, off_t);

extern int munmap(void *, size_t);

extern int posix_madvise(void *, size_t, int);

#ifdef __cplusplus
}
#endif

#endif /* BEZOS_POSIX_SYS_MMAN_H */
