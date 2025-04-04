#pragma once

#include "arch/generic/stack.hpp"

namespace arch {
    struct StackWalkX86_64 : GenericStackWalk {
        template<typename F>
        static void Walk(void **frame, F&& callback) {
            while (frame && frame[0]) {
                void **next = (void**)frame[0];
                void *pc = frame[1];

                callback(next, pc);

                if (next <= frame) break;
                frame = next;
            }
        }
    };

    using StackWalk = StackWalkX86_64;
}
