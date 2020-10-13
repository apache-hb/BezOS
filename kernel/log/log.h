#pragma once

#include <util.h>

namespace log {
    struct channel {
        virtual void info(const char *str) = 0;
        virtual void fatal(const char *str) = 0;
    };

    struct vga : channel {
        virtual void info(const char *str) override;
        virtual void fatal(const char *str) override;

    private:
        u32 idx = 0;
    };

    void init(channel *stream);
    void info(const char *str);
    void fatal(const char *str);
}
