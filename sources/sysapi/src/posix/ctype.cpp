#include <posix/ctype.h>

int isascii(int c) noexcept {
    return c >= 0 && c <= 0x7F;
}

int isprint(int c) noexcept {
    return c >= 0x20 && c <= 0x7E;
}

int isdigit(int c) noexcept {
    return c >= '0' && c <= '9';
}

int isxdigit(int c) noexcept {
    return isdigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

int isalnum(int c) noexcept {
    return isdigit(c) || isalpha(c);
}

int isalpha(int c) noexcept {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

int isupper(int c) noexcept {
    return c >= 'A' && c <= 'Z';
}

int islower(int c) noexcept {
    return c >= 'a' && c <= 'z';
}

int toupper(int c) noexcept {
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 'A';
    }

    return c;
}

int tolower(int c) noexcept {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 'a';
    }

    return c;
}

int isspace(int c) noexcept {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

int ispunct(int c) noexcept {
    return (c >= '!' && c <= '/') || (c >= ':' && c <= '@') || (c >= '[' && c <= '`') || (c >= '{' && c <= '~');
}

int iscntrl(int c) noexcept {
    return c < 0x20 || c == 0x7F;
}

int isgraph(int c) noexcept {
    return c >= 0x21 && c <= 0x7E;
}
