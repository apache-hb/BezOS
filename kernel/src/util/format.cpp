#include "util/format.hpp"

using HexDumpFormat = km::Format<km::HexDump>;

using namespace stdx::literals;

/// @brief Is the given byte printable and not a control character.
static bool IsText(uint8_t c) {
    return c >= 0x20 && c <= 0x7E;
}

static constexpr size_t kRowSize = 16;

static void FormatRow(km::IOutStream& out, std::span<const std::byte> row) {
    uintptr_t address = (uintptr_t)row.data();

    out.write(km::format(km::Hex(address).pad(16, '0')));
    out.write(" : "_sv);

    for (size_t i = 0; i < kRowSize; i++) {
        if (i >= row.size()) {
            out.write("  "_sv);
        } else {
            out.write(km::FormatInt((uint8_t)row[i], 16, 2, '0'));
        }

        if (i % 2 == 1) {
            out.write(" "_sv);
        }
    }

    out.write("  "_sv);

    for (size_t i = 0; i < kRowSize; i++) {
        if (i >= row.size()) {
            out.write(" "_sv);
        } else {
            uint8_t c = (uint8_t)row[i];
            out.write(IsText(c) ? (char)c : '.');
        }
    }
}

void HexDumpFormat::format(IOutStream &out, HexDump value) {
    size_t size = value.data.size();
    size_t start = 0;

    while ((start + kRowSize) < size) {
        FormatRow(out, value.data.subspan(start, kRowSize));
        out.write("\n"_sv);
        start += kRowSize;
    }

    FormatRow(out, value.data.subspan(start));
}
