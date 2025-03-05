#include <posix/ctype.h>

int isprint(int c) noexcept {
    return c >= 0x20 && c <= 0x7E;
}

int isdigit(int c) noexcept {
    return c >= '0' && c <= '9';
}

int isalnum(int c) noexcept {
    return isdigit(c) || isalpha(c);
}

int isalpha(int c) noexcept {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
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
