#pragma once

#include "memory/allocator.hpp"

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

    template<typename F>
    void WalkStackFramesChecked(km::PageTableManager& pt, void **frame, F&& callback) {
        while (true) {
            if (frame == nullptr) {
                callback(nullptr, nullptr, "End of stack");
                break;
            }

            if (pt.getMemoryFlags(frame) == km::PageFlags::eNone) {
                callback(frame, nullptr, "Invalid stack frame");
                break;
            }

            if (pt.getMemoryFlags(*frame) == km::PageFlags::eNone) {
                callback(frame, *frame, "Invalid stack frame");
                break;
            }

            void **next = (void**)frame[0];
            void *pc = frame[1];

            callback(next, pc, "");

            if (next <= frame) break;
            frame = next;
        }
    }
}
