#ifndef BEZOS_POSIX_SYS_WAIT_H
#define BEZOS_POSIX_SYS_WAIT_H 1

#include <detail/id.h>
#include <detail/cxx.h>

BZP_API_BEGIN

#define WNOHANG (1 << 0)
#define WUNTRACED (1 << 1)
#define WCONTINUED (1 << 2)

extern pid_t wait(int *) BZP_NOEXCEPT;
extern pid_t waitpid(pid_t, int *, int) BZP_NOEXCEPT;

BZP_API_END

#endif /* BEZOS_POSIX_SYS_WAIT_H */
