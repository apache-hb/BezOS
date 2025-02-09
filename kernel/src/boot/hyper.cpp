#include "boot.hpp"

#include <ultra_protocol.h>

static constexpr size_t kStackSize = sm::kilobytes(16).bytes();
static constexpr size_t kBootMemory = sm::kilobytes(64).bytes();
static constexpr size_t kLowMemory = sm::megabytes(1).bytes();

template<typename T>
static const T *GetAttribute(const ultra_boot_context *context, uint32_t type) {
    uint32_t i = 0;
    const ultra_attribute_header *header = &context->attributes[0];

    while (i < context->attribute_count) {
        if (header->type == type) {
            return reinterpret_cast<const T*>(header);
        }

        header = ULTRA_NEXT_ATTRIBUTE(header);
    }

    return nullptr;
}

extern "C" int HyperMain(ultra_boot_context *context, uint32_t magic) {
    const char *base = (char*)__builtin_frame_address(0) + (sizeof(void*) * 2);
    auto framebuffer = GetAttribute<ultra_framebuffer_attribute>(context, ULTRA_ATTRIBUTE_FRAMEBUFFER_INFO);
    auto platformInfo = GetAttribute<ultra_platform_info_attribute>(context, ULTRA_ATTRIBUTE_PLATFORM_INFO);
    auto kernelInfo = GetAttribute<ultra_kernel_info_attribute>(context, ULTRA_ATTRIBUTE_KERNEL_INFO);
    auto memmap = GetAttribute<ultra_memory_map_attribute>(context, ULTRA_ATTRIBUTE_MEMORY_MAP);

    while (1) { }
}
