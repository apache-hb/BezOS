#include "new_shim.hpp"

static IGlobalAllocator *gGlobalAllocator = nullptr;

IGlobalAllocator *GetGlobalAllocator() {
    if (gGlobalAllocator == nullptr) {
        static IGlobalAllocator sDefaultAllocator;
        return &sDefaultAllocator;
    } else {
        return gGlobalAllocator;
    }
}

void SetGlobalAllocator(IGlobalAllocator *allocator) {
    gGlobalAllocator = allocator;
}
