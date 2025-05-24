#pragma once

#include <stdint.h>

namespace sm::detail {
    using CounterInt = uint32_t;

    enum class EjectAction {
        eNone,
        eDelay,
        eDestroy,
    };
}
