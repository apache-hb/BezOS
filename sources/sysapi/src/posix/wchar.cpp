#include "src/posix/private.hpp"
#include <posix/wchar.h>

wchar_t *wmemchr(const wchar_t *s, wchar_t c, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (s[i] == c) {
            return const_cast<wchar_t*>(&s[i]);
        }
    }
    return nullptr;
}

int wmemcmp(const wchar_t *lhs, const wchar_t *rhs, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (lhs[i] != rhs[i]) {
            return lhs[i] - rhs[i];
        }
    }
    return 0;
}

size_t wcslen(const wchar_t *str) {
    size_t len = 0;
    while (*str++) {
        len++;
    }
    return len;
}

int swprintf(wchar_t *, size_t, const wchar_t *, ...) noexcept {
    Unimplemented();
    return -1;
}

int iswspace(wint_t c) {
    return c == L' ' || c == L'\t' || c == L'\n' || c == L'\v' || c == L'\f' || c == L'\r';
}

namespace {
template<typename T>
T wcstoAny(const wchar_t *str, wchar_t **end, int base) noexcept {
    T result = 0;
    bool negative = false;

    while (iswspace(*str)) {
        str++;
    }

    if (*str == L'-') {
        negative = true;
        str++;
    } else if (*str == L'+') {
        str++;
    }

    if (base == 0) {
        if (*str == L'0') {
            str++;
            if (*str == L'x' || *str == L'X') {
                base = 16;
                str++;
            } else {
                base = 8;
            }
        } else {
            base = 10;
        }
    }

    while (true) {
        wchar_t c = *str;
        int digit = 0;

        if (c >= L'0' && c <= L'9') {
            digit = c - L'0';
        } else if (c >= L'a' && c <= L'z') {
            digit = c - L'a' + 10;
        } else if (c >= L'A' && c <= L'Z') {
            digit = c - L'A' + 10;
        } else {
            break;
        }

        if (digit >= base) {
            break;
        }

        result = result * base + digit;
        str++;
    }

    if (end) {
        *end = const_cast<wchar_t*>(str);
    }

    return negative ? -result : result;
}
}

long wcstol(const wchar_t *str, wchar_t **end, int base) noexcept {
    return wcstoAny<long>(str, end, base);
}

long long wcstoll(const wchar_t *str, wchar_t **end, int base) noexcept {
    return wcstoAny<long long>(str, end, base);
}

unsigned long wcstoul(const wchar_t *str, wchar_t **end, int base) noexcept {
    return wcstoAny<unsigned long>(str, end, base);
}

unsigned long long wcstoull(const wchar_t *str, wchar_t **end, int base) noexcept {
    return wcstoAny<unsigned long long>(str, end, base);
}

double wcstod(const wchar_t *, wchar_t **) noexcept {
    Unimplemented();
    return 0.0;
}

long double wcstold(const wchar_t *, wchar_t **) noexcept {
    Unimplemented();
    return 0.0;
}

float wcstof(const wchar_t *, wchar_t **) noexcept {
    Unimplemented();
    return 0.0;
}
