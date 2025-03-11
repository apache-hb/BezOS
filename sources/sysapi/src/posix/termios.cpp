#include <posix/termios.h>

#include "private.hpp"

speed_t cfgetispeed(const struct termios *) BZP_NOEXCEPT {
    Unimplemented();
    return 0;
}

speed_t cfgetospeed(const struct termios *) BZP_NOEXCEPT {
    Unimplemented();
    return 0;
}

int cfsetispeed(struct termios *, speed_t) BZP_NOEXCEPT {
    Unimplemented();
    return -1;
}

int cfsetospeed(struct termios *, speed_t) BZP_NOEXCEPT {
    Unimplemented();
    return -1;
}

int tcdrain(int) BZP_NOEXCEPT {
    Unimplemented();
    return -1;
}

int tcflow(int, int) BZP_NOEXCEPT {
    Unimplemented();
    return -1;
}

int tcflush(int, int) BZP_NOEXCEPT {
    Unimplemented();
    return -1;
}

int tcgetattr(int, struct termios *) BZP_NOEXCEPT {
    Unimplemented();
    return -1;
}

pid_t tcgetsid(int) BZP_NOEXCEPT {
    Unimplemented();
    return -1;
}

int tcsendbreak(int, int) BZP_NOEXCEPT {
    Unimplemented();
    return -1;
}

int tcsetattr(int, int, const struct termios *) BZP_NOEXCEPT {
    Unimplemented();
    return -1;
}
