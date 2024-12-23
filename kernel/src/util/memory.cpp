#include "util/memory.hpp"
#include "util/format.hpp"

#include "std/traits.hpp"

#include <string.h>

stdx::StringView km::Memory::toString(char buffer[kStringSize]) const {
    if (mBytes == 0) {
        strcpy(buffer, "0b");
        return stdx::StringView(buffer, buffer + 2);
    }

    size_t total = mBytes;

    // seperate each part with a +

    char num[stdx::NumericTraits<size_t>::kMaxDigits10];

    char *ptr = buffer;

    for (int fmt = eCount - 1; fmt >= 0; fmt--) {
        size_t size = total / kSizes[fmt];
        if (size > 0) {
            stdx::StringView result = FormatInt(num, size, 10);
            memcpy(ptr, result.data(), result.count());
            ptr += result.count();
            strcpy(ptr, kNames[fmt]);
            ptr += kNames[fmt][0] == 'b' ? 1 : 2;

            total %= kSizes[fmt];

            if (total > 0) {
                memcpy(ptr, "+", 1);
                ptr += 1;
            }
        }
    }

    return stdx::StringView(buffer, ptr);
}
