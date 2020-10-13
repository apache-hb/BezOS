#include "log.h"

namespace log {
    channel *out = nullptr;
    void init(channel *stream) {
        out = stream;
        out->info("here");

        ((u16*)0xB8000)[0] = 'x' | 7 << 8;
    }

    void info(const char *str) {
        ((u16*)0xB8000)[0] = 'y' | 7 << 8;
        out->info(str);
    }

    void fatal(const char *str) {
        out->fatal(str);
    }
}
