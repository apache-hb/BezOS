#pragma once

namespace x64 {
    template<typename F>
    void WalkStackFrames(void **frame, F&& callback) {
        while (frame && frame[0]) {
            void **next = (void**)frame[0];
            void *pc = frame[1];

            callback(next, pc);

            if (next <= frame) break;
            frame = next;
        }
    }
}
