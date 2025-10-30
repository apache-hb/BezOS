#ifndef BEZOS_POSIX_TERMIOS_H
#define BEZOS_POSIX_TERMIOS_H 1

#include <detail/cxx.h>
#include <detail/id.h>

BZP_API_BEGIN

#ifndef NCCS
#   define NCCS 32
#endif

/* input modes */

#define ICRNL (1 << 0)
#define INLCR (1 << 1)
#define IGNCR (1 << 2)
#define IXON (1 << 3)
#define PARMRK (1 << 4)
#define BRKINT (1 << 5)
#define ISTRIP (1 << 6)
#define IGNBRK (1 << 7)
#define IGNPAR (1 << 8)
#define INPCK (1 << 9)
#define IXOFF (1 << 10)

/* output modes */

#define OPOST (1 << 0)

/* control modes */

#define CSIZE (1 << 0)
#define CS5 0
#define CS6 1
#define CS7 2
#define CS8 3

#define CSTOPB (1 << 1)
#define CREAD (1 << 2)
#define PARENB (1 << 3)
#define CLOCAL (1 << 4)
#define HUPCL (1 << 5)
#define PARODD (1 << 6)

/* line control */

#define TCIFLUSH 0x1
#define TCIOFLUSH 0x3
#define TCOFLUSH 0x2

/* local modes */

#define ECHO (1 << 0)
#define ECHONL (1 << 1)
#define ICANON (1 << 2)
#define IEXTEN (1 << 3)
#define ISIG (1 << 4)
#define NOFLSH (1 << 5)
#define ECHOE (1 << 6)
#define ECHOK (1 << 7)

/* attributes */

#define TCSANOW 0
#define TCSADRAIN 1
#define TCSAFLUSH 2

/* baud */

#define B0 0
#define B50 50
#define B75 75
#define B110 110
#define B134 134
#define B150 150
#define B200 200
#define B300 300
#define B600 600
#define B1200 1200
#define B1800 1800
#define B2400 2400
#define B4800 4800
#define B9600 9600
#define B19200 19200
#define B38400 38400

/* c_cc indices */

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

extern speed_t cfgetispeed(const struct termios *);
extern speed_t cfgetospeed(const struct termios *);
extern int cfsetispeed(struct termios *, speed_t);
extern int cfsetospeed(struct termios *, speed_t);
extern int tcdrain(int);
extern int tcflow(int, int);
extern int tcflush(int, int);
extern int tcgetattr(int, struct termios *);
extern pid_t tcgetsid(int);
extern int tcsendbreak(int, int);
extern int tcsetattr(int, int, const struct termios *);

BZP_API_END

#endif /* BEZOS_POSIX_TERMIOS_H */
