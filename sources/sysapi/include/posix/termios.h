#ifndef BEZOS_POSIX_TERMIOS_H
#define BEZOS_POSIX_TERMIOS_H 1

#include <detail/cxx.h>
#include <detail/id.h>

BZP_API_BEGIN

#ifndef NCCS
#   define NCCS 32
#endif

#define VEOF 0
#define VEOL 1
#define VERASE 2
#define VINTR 3
#define VKILL 4
#define VMIN 5
#define VQUIT 6
#define VSTART 7
#define VSTOP 8
#define VSUSP 9
#define VTIME 10

typedef unsigned char cc_t;
typedef unsigned speed_t;
typedef unsigned tcflag_t;

struct termios {
    tcflag_t c_iflag;
    tcflag_t c_oflag;
    tcflag_t c_cflag;
    tcflag_t c_lflag;
    cc_t c_cc[NCCS];
    speed_t c_ispeed;
    speed_t c_ospeed;
};

extern speed_t cfgetispeed(const struct termios *) BZP_NOEXCEPT;
extern speed_t cfgetospeed(const struct termios *) BZP_NOEXCEPT;
extern int cfsetispeed(struct termios *, speed_t) BZP_NOEXCEPT;
extern int cfsetospeed(struct termios *, speed_t) BZP_NOEXCEPT;
extern int tcdrain(int) BZP_NOEXCEPT;
extern int tcflow(int, int) BZP_NOEXCEPT;
extern int tcflush(int, int) BZP_NOEXCEPT;
extern int tcgetattr(int, struct termios *) BZP_NOEXCEPT;
extern pid_t tcgetsid(int) BZP_NOEXCEPT;
extern int tcsendbreak(int, int) BZP_NOEXCEPT;
extern int tcsetattr(int, int, const struct termios *) BZP_NOEXCEPT;

BZP_API_END

#endif /* BEZOS_POSIX_TERMIOS_H */
