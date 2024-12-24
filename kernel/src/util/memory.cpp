#include "util/memory.hpp"
#include "util/format.hpp"

#include "std/traits.hpp"

#include <string.h>

static constexpr const char *kNames[km::Memory::eCount] = {
    [km::Memory::eBytes] = "b",
    [km::Memory::eKilobytes] = "kb",
    [km::Memory::eMegabytes] = "mb",
    [km::Memory::eGigabytes] = "gb",
    [km::Memory::eTerabytes] = "tb"
};

stdx::StringView km::toString(char buffer[Memory::kStringSize], Memory value) {
    size_t bytes = value.asBytes();
    if (bytes == 0) {
        strcpy(buffer, "0b");
        return stdx::StringView(buffer, buffer + 2);
    }

    size_t total = bytes;

    // seperate each part with a +

    char num[stdx::NumericTraits<size_t>::kMaxDigits10];

    char *ptr = buffer;

    for (int fmt = Memory::eCount - 1; fmt >= 0; fmt--) {
        size_t size = total / Memory::kSizes[fmt];
        if (size > 0) {
            stdx::StringView result = FormatInt(num, size, 10);
            memcpy(ptr, result.data(), result.count());
            ptr += result.count();
            strcpy(ptr, kNames[fmt]);
            ptr += kNames[fmt][0] == 'b' ? 1 : 2;

            total %= Memory::kSizes[fmt];

            if (total > 0) {
                memcpy(ptr, "+", 1);
                ptr += 1;
            }
        }
    }

    return stdx::StringView(buffer, ptr);
}
