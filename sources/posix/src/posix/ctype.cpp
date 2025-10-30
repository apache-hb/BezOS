#include <posix/ctype.h>

int isascii(int c) {
    return c >= 0 && c <= 0x7F;
}

int isprint(int c) {
    return c >= 0x20 && c <= 0x7E;
}

int isdigit(int c) {
    return c >= '0' && c <= '9';
}

int isxdigit(int c) {
    return isdigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

int isalnum(int c) {
    return isdigit(c) || isalpha(c);
}

int isalpha(int c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

int isupper(int c) {
    return c >= 'A' && c <= 'Z';
}

int islower(int c) {
    return c >= 'a' && c <= 'z';
}

int toupper(int c) {
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 'A';
    }

    return c;
}

int tolower(int c) {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 'a';
    }

    return c;
}

int isspace(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

int ispunct(int c) {
    return (c >= '!' && c <= '/') || (c >= ':' && c <= '@') || (c >= '[' && c <= '`') || (c >= '{' && c <= '~');
}

int iscntrl(int c) {
    return c < 0x20 || c == 0x7F;
}

int isgraph(int c) {
    return c >= 0x21 && c <= 0x7E;
}
