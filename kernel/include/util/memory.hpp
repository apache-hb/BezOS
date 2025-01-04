#pragma once

#include "std/string_view.hpp"

#include "util/format.hpp"

namespace sm {
    struct Memory {
        enum Unit {
            eBytes,
            eKilobytes,
            eMegabytes,
            eGigabytes,
            eTerabytes,

            eCount
        };

        static constexpr size_t kByte = 1;
        static constexpr size_t kKilobyte = kByte * 1024;
        static constexpr size_t kMegabyte = kKilobyte * 1024;
        static constexpr size_t kGigabyte = kMegabyte * 1024;
        static constexpr size_t kTerabyte = kGigabyte * 1024;

        static constexpr size_t kStringSize = 64;

        static constexpr size_t kSizes[eCount] = {
            kByte,
            kKilobyte,
            kMegabyte,
            kGigabyte,
            kTerabyte
        };

        constexpr Memory(size_t memory = 0, Unit unit = eBytes)
            : mBytes(memory * kSizes[unit])
        { }

        constexpr size_t bytes() const { return mBytes; }
        constexpr size_t kilobytes() const { return mBytes / kKilobyte; }
        constexpr size_t megabytes() const { return mBytes / kMegabyte; }
        constexpr size_t gigabytes() const { return mBytes / kGigabyte; }
        constexpr size_t terabytes() const { return mBytes / kTerabyte; }

    private:
        size_t mBytes;
    };

    stdx::StringView toString(char buffer[Memory::kStringSize], Memory value);

    constexpr Memory bytes(size_t bytes) { return Memory(bytes, Memory::eBytes); }
    constexpr Memory kilobytes(size_t kilobytes) { return Memory(kilobytes, Memory::eKilobytes); }
    constexpr Memory megabytes(size_t megabytes) { return Memory(megabytes, Memory::eMegabytes); }
    constexpr Memory gigabytes(size_t gigabytes) { return Memory(gigabytes, Memory::eGigabytes); }
    constexpr Memory terabytes(size_t terabytes) { return Memory(terabytes, Memory::eTerabytes); }
}

template<>
struct km::StaticFormat<sm::Memory> {
    static constexpr size_t kStringSize = 64;
    static stdx::StringView toString(char *buffer, sm::Memory value) {
        return sm::toString(buffer, value);
    }
};
