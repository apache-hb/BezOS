#ifndef BEZOS_POSIX_PWD_H
#define BEZOS_POSIX_PWD_H 1

#include <detail/id.h>

#ifdef __cplusplus
extern "C" {
#endif

struct passwd {
    char *pw_name;
    uid_t pw_uid;
    gid_t pw_gid;
    char *pw_dir;
    char *pw_shell;
};

extern struct passwd *getpwuid(uid_t);

#ifdef __cplusplus
}
#endif

#endif /* BEZOS_POSIX_PWD_H */
