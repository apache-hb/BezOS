#pragma once

#include "util/format.hpp"

namespace km {
    enum class Align {
        eLeft,
        eRight,
    };

    struct FormatConfig {
        Align align = Align::eRight;
        unsigned width = 0;
        char fill = ' ';
    };

    template<IsFormatSize T>
    struct FormatObject {
        T value;
        FormatConfig config;
    };

    struct FormatBuilder {
        FormatConfig config;

        template<typename T>
        FormatObject<T> operator+(T value) const {
            return { value, config };
        }
    };

    constexpr FormatBuilder lpad(unsigned width, char fill = ' ') {
        return { { Align::eLeft, width, fill } };
    }

    constexpr FormatBuilder rpad(unsigned width, char fill = ' ') {
        return { { Align::eRight, width, fill } };
    }

    template<typename T>
    struct Format<FormatObject<T>> {
        static void format(IOutStream& out, const FormatObject<T>& value) {
            stdx::StaticString item = km::format(value.value);
            if (value.config.width <= item.count()) {
                out.write(item);
                return;
            }

            if (value.config.align == Align::eRight) {
                for (unsigned i = item.count(); i < value.config.width; i++) {
                    out.write(value.config.fill);
                }
                out.write(item);
            } else {
                out.write(item);
                for (unsigned i = item.count(); i < value.config.width; i++) {
                    out.write(value.config.fill);
                }
            }
        }
    };
}
