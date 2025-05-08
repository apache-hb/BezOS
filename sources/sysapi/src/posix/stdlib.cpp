#include <atomic>
#include <iterator>
#include <posix/stdlib.h>

#include <posix/errno.h>
#include <posix/assert.h>
#include <posix/unistd.h>
#include <posix/string.h>
#include <posix/ctype.h>

#include <bezos/facility/process.h>

#include <rpmalloc/rpmalloc.h>

#include "private.hpp"

namespace {
struct InitMalloc {
    InitMalloc() {
        int err = rpmalloc_initialize(nullptr);
        assert(err == 0);
    }

    ~InitMalloc() {
        rpmalloc_finalize();
    }

    void *malloc(size_t size) {
        return rpmalloc(size);
    }

    void *aligned_alloc(size_t alignment, size_t size) {
        return rpaligned_alloc(alignment, size);
    }

    void *calloc(size_t n, size_t size) {
        return rpcalloc(n, size);
    }

    void *realloc(void *ptr, size_t size) {
        return rprealloc(ptr, size);
    }

    void free(void *ptr) {
        rpfree(ptr);
    }
};

InitMalloc& GlobalMalloc() {
    static InitMalloc sMalloc;
    return sMalloc;
}
}

__attribute__((__nothrow__, __noreturn__))
void abort(void) {
    exit(EXIT_FAILURE);
}

__attribute__((__nothrow__, __noreturn__))
void exit(int exitcode) {
    OsProcessHandle handle = OS_HANDLE_INVALID;
    OsStatus status = OsProcessCurrent(eOsProcessAccessDestroy, &handle);
    assert(status == OsStatusSuccess);

    //
    // OsProcessTerminate when called with the current process handle
    // will terminate the current process and never return.
    //
    status = OsProcessTerminate(handle, exitcode);
    __builtin_unreachable();
}

int abs(int v) {
    return v < 0 ? -v : v;
}

long labs(long v) {
    return v < 0 ? -v : v;
}

long long llabs(long long v) {
    return v < 0 ? -v : v;
}

double atof(const char *) {
    Unimplemented();
    return 0.0;
}

int atoi(const char *) {
    Unimplemented();
    return 0;
}

long int atol(const char *) {
    Unimplemented();
    return 0;
}

long long int atoll(const char *) {
    Unimplemented();
    return 0;
}

double strtod(const char *, char **) {
    Unimplemented();
    return 0.0;
}

float strtof(const char *, char **) {
    Unimplemented();
    return 0.0f;
}

template<typename T>
static T strtoAny(const char *str, char **end, int base) {
    while (isspace(*str)) {
        str += 1;
    }

    bool negative = false;
    if (*str == '-') {
        negative = true;
        str += 1;
    } else if (*str == '+') {
        str += 1;
    }

    if (base == 0) {
        if (*str == '0') {
            if (str[1] == 'x' || str[1] == 'X') {
                base = 16;
                str += 2;
            } else {
                base = 8;
            }
        } else {
            base = 10;
        }
    }

    T result = 0;
    while (true) {
        int digit = *str;
        if (isdigit(digit)) {
            digit -= '0';
        } else if (isalpha(digit)) {
            digit = tolower(digit) - 'a' + 10;
        } else {
            break;
        }

        if (digit >= base) {
            break;
        }

        result = result * base + digit;
        str += 1;
    }

    if (end) {
        *end = const_cast<char*>(str);
    }

    return negative ? -result : result;
}

long strtol(const char *str, char **end, int base) {
    return strtoAny<long>(str, end, base);
}

long long strtoll(const char *str, char **end, int base) {
    return strtoAny<long long>(str, end, base);
}

unsigned long strtoul(const char *str, char **end, int base) {
    Unimplemented();
    return 0;
}

unsigned long long strtoull(const char *, char **, int) {
    Unimplemented();
    return 0;
}

long double strtold(const char *, char **) {
    Unimplemented();
    return 0.0;
}

__attribute__((__nothrow__, __malloc__, __alloc_size__(1), __allocating__))
void *malloc(size_t size) {
    return GlobalMalloc().malloc(size);
}

__attribute__((__nothrow__, __malloc__, __alloc_size__(2), __allocating__))
void *aligned_alloc(size_t alignment, size_t size) {
    return GlobalMalloc().aligned_alloc(alignment, size);
}

__attribute__((__nothrow__, __malloc__, __alloc_size__(1, 2), __allocating__))
void *calloc(size_t n, size_t size) {
    return GlobalMalloc().calloc(n, size);
}

__attribute__((__nothrow__, __malloc__, __alloc_size__(2), __allocating__))
void *realloc(void *ptr, size_t size) {
    return GlobalMalloc().realloc(ptr, size);
}

__attribute__((__nothrow__, __nonnull__, __nonallocating__))
void free(void *ptr) {
    return GlobalMalloc().free(ptr);
}

void qsort(void *, size_t, size_t, int (*)(const void *, const void *)) {
    Unimplemented();
}

char *getenv(const char *name) {
    Unimplemented();
    DebugLog(eOsLogInfo, "POSIX getenv: '%s'", name);

    char **iter = environ;
    size_t len = strlen(name);
    while (*iter) {
        if (strncmp(*iter, name, len) == 0) {
            return const_cast<char*>(*iter + len + 1);
        }

        iter += 1;
    }

    errno = ENOENT;
    return nullptr;
}

extern int putenv(char *) {
    Unimplemented();
    return -1;
}

extern int setenv(const char *name, const char *val, int overwrite) {
    Unimplemented();
    DebugLog(eOsLogInfo, "POSIX setenv: '%s'='%s'%s", name, val, overwrite ? " (overwrite)" : "");
    return -1;
}

extern int unsetenv(const char *) {
    Unimplemented();
    return -1;
}

void srand(unsigned) {
    Unimplemented();
}

int rand(void) {
    Unimplemented();
    return 0;
}

int rand_r(unsigned *) {
    Unimplemented();
    return 0;
}

char *mkdtemp(char *) {
    Unimplemented();
    return nullptr;
}

int mkstemp(char *) {
    Unimplemented();
    return -1;
}

char *mktemp(char *) {
    Unimplemented();
    return nullptr;
}

int atexit(void (*)(void)) {
    Unimplemented();
    return -1;
}

struct CxaExitNode {
    CxaExitNode *next;
    void (*callback)(void *);
    void *object;
};

// A small static buffer of nodes to initially allocate from.
// This isnt just an optimization, its needed for correctness. `malloc` does some static init
// and if that calls `__cxa_atexit` it will deadlock if we use `malloc` to allocate the node.
static CxaExitNode gExitNodeBuffer[16];
static std::atomic<size_t> gExitNodeIndex{0};

static CxaExitNode *TakeExitNode() {
    size_t index = gExitNodeIndex.fetch_add(1);
    if (index >= std::size(gExitNodeBuffer)) {
        return nullptr;
    }

    return &gExitNodeBuffer[index];
}

static CxaExitNode *gHeadNode;

extern "C" int __cxa_atexit(void (*callback)(void *), void *object, void *dso) {
    if (callback == nullptr) {
        return -1;
    }

    if (CxaExitNode *node = TakeExitNode()) {
        *node = CxaExitNode {
            .next = gHeadNode,
            .callback = callback,
            .object = object,
        };
        gHeadNode = node;
        return 0;
    }

    CxaExitNode *node = new (malloc(sizeof(CxaExitNode))) CxaExitNode {
        .next = gHeadNode,
        .callback = callback,
        .object = object,
    };

    if (!node) {
        return -1;
    }

    gHeadNode = node;

    return 0;
}

extern "C" void *__dso_handle = nullptr;
