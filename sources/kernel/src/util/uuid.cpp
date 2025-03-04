#include "util/uuid.hpp"

using namespace sm;

#if 0
namespace chrono = std::chrono;
#endif

using namespace std::chrono_literals;

#if 0
static constexpr std::chrono::sys_days kGregorianReform = std::chrono::sys_days{1582y/10/15};
#endif

// time handling

#if 0
detail::rfc9562_clock::time_point detail::rfc9562_clock::from_sys(std::chrono::system_clock::time_point time) {
    // our epoch is 1582-10-15T00:00:00Z, the gregorian reform
    // we need to convert from the system clock epoch to the rfc9562 epoch.

    rfc9562_clock::duration duration = std::chrono::floor<rfc9562_clock::duration>(time - kGregorianReform);

    return rfc9562_clock::time_point{duration};
}

std::chrono::system_clock::time_point detail::rfc9562_clock::to_sys(time_point time) {
    return std::chrono::system_clock::time_point{std::chrono::duration_cast<std::chrono::system_clock::duration>((kGregorianReform.time_since_epoch() + time.time_since_epoch()))};
}

// v1

static constexpr uint16_t kClockSeqMask = 0b0011'1111'1111'1111;

uuid uuid::v1(std::chrono::utc_clock::time_point time, uint16_t clockSeq, MacAddress node) {
    detail::rfc9562_clock::time_point clockOffset = chrono::clock_cast<detail::rfc9562_clock>(time);

    uint64_t intervals = clockOffset.time_since_epoch().count();

    uint16_t uuidClockSeq
        = (uuid::eDCE << 14) // 64:65
        | (clockSeq & kClockSeqMask); // 66:79

    uuidv1 result = {
        .time0 = intervals,
        .time1 = intervals >> 32,
        .time2 = ((intervals >> 48) & 0x0FFF) | (uuid::eVersion1 << 12),
        .clockSeq = uuidClockSeq,
        .node = node,
    };

    return std::bit_cast<uuid>(result);
}

uint16_t uuid::v1ClockSeq() const noexcept {
    return uv1.clockSeq & kClockSeqMask;
}

MacAddress uuid::v1Node() const noexcept {
    return uv1.node;
}

std::chrono::utc_clock::time_point uuid::v1Time() const noexcept {
    uint64_t intervals
        = uint64_t(uv1.time2 & 0x0FFF) << 48
        | uint64_t(uv1.time1) << 32
        | uint64_t(uv1.time0);

    return chrono::clock_cast<chrono::utc_clock>(detail::rfc9562_clock::time_point{detail::rfc9562_clock::duration{intervals}});
}

// v6

uuid uuid::v6(std::chrono::utc_clock::time_point time, uint16_t clockSeq, MacAddress node) {
    detail::rfc9562_clock::time_point clockOffset = chrono::clock_cast<detail::rfc9562_clock>(time);

    uint64_t intervals = clockOffset.time_since_epoch().count();

    uint16_t uuidClockSeq
        = (uuid::eDCE << 14) // 64:65
        | (clockSeq & kClockSeqMask); // 66:79

    uuidv6 result = {
        .time1 = uint48_t(intervals >> 12),
        .time0 = (intervals & 0x0FFF) | (uuid::eVersion6 << 12),
        .clockSeq = uuidClockSeq,
        .node = node,
    };

    return std::bit_cast<uuid>(result);
}

uint16_t uuid::v6ClockSeq() const noexcept {
    return uv6.clockSeq & kClockSeqMask;
}

MacAddress uuid::v6Node() const noexcept {
    return uv6.node;
}

std::chrono::utc_clock::time_point uuid::v6Time() const noexcept {
    uint64_t intervals
        = uint64_t(uv6.time1.load()) << 12
        | uint64_t(uv6.time0 & 0x0FFF);

    return chrono::clock_cast<chrono::utc_clock>(detail::rfc9562_clock::time_point{detail::rfc9562_clock::duration{intervals}});
}

// v7

uuid uuid::v7(std::chrono::system_clock::time_point time, const uint8_t random[10]) {
    long long ts = chrono::duration_cast<chrono::milliseconds>(time.time_since_epoch()).count();

    // TODO: this may be a touch innacurate
    uuidv7 result = {
        .time = uint48_t(ts),
    };

    memcpy(result.rand, random, sizeof(result.rand));
    result.rand[0] = (result.rand[0] & 0b0000'1111) | uuid::eVersion7 << 4;
    result.rand[2] = (result.rand[2] & 0b0011'1111) | uuid::eDCE << 6;

    return std::bit_cast<uuid>(result);
}

std::chrono::system_clock::time_point uuid::v7Time() const noexcept {
    uint64_t ts = uv7.time.load();

    return chrono::system_clock::time_point{chrono::milliseconds{ts}};
}
#endif

// string formatting

void uuid::strfuid(char dst[kStringSize], uuid uuid) noexcept {
    auto writeOctets = [&](size_t start, size_t n, size_t o) {
        constexpr char kHex[] = "0123456789abcdef";
        for (size_t i = start; i < n; i++) {
            dst[i * 2 + o + 0] = kHex[uuid.octets[i] >> 4];
            dst[i * 2 + o + 1] = kHex[uuid.octets[i] & 0x0F];
        }
    };

    writeOctets(0, 4, 0);
    dst[8] = '-';
    writeOctets(4, 6, 1);
    dst[13] = '-';
    writeOctets(6, 8, 2);
    dst[18] = '-';
    writeOctets(8, 10, 3);
    dst[23] = '-';
    writeOctets(10, 16, 4);
}

// parsing

static bool isHex(char c) {
    return (c >= '0' && c <= '9')
        || (c >= 'a' && c <= 'f')
        || (c >= 'A' && c <= 'F');
}

static uint8_t fromHex(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }

    return 0;
}

static bool readOctets(const char *str, size_t n, uint8_t *dst) {
    for (size_t i = 0; i < n; i++) {
        char c0 = str[(i * 2) + 0];
        char c1 = str[(i * 2) + 1];

        if (!isHex(c0) || !isHex(c1)) {
            return false;
        }

        dst[i] = (fromHex(c0) << 4) | fromHex(c1);
    }

    return true;
}

bool uuid::parse(const char str[kStringSize], uuid& result) noexcept {
    uuid tmp = uuid::nil();

    if (!readOctets(str, 4, tmp.octets)) return false;
    if (str[8] != '-') return false;

    if (!readOctets(str + 9, 2, tmp.octets + 4)) return false;
    if (str[13] != '-') return false;

    if (!readOctets(str + 14, 2, tmp.octets + 6)) return false;
    if (str[18] != '-') return false;

    if (!readOctets(str + 19, 2, tmp.octets + 8)) return false;
    if (str[23] != '-') return false;

    if (!readOctets(str + 24, 6, tmp.octets + 10)) return false;

    result = tmp;
    return true;
}

bool uuid::parseMicrosoft(const char str[kMicrosoftStringSize], uuid& result) noexcept {
    if (str[0] != '{') return false;
    if (str[37] != '}') return false;

    return parse(str + 1, result);
}

bool uuid::parseHex(const char str[kHexStringSize], uuid &result) noexcept {
    uuid tmp = uuid::nil();

    if (!readOctets(str, 16, tmp.octets)) return false;

    result = tmp;
    return true;
}

bool uuid::parseAny(const char str[kMaxStringSize], uuid& result) noexcept {
    // probably a microsoft uuid
    if (str[0] == '{' && str[37] == '}') {
        return parseMicrosoft(str, result);
    }

    // probably an 8-4-4-4-12 uuid
    if (str[8] == '-') {
        return parse(str, result);
    }

    // best guess is a raw hex uuid
    return parseHex(str, result);
}
