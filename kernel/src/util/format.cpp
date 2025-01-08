#include "util/format.hpp"

using HexDumpFormat = km::Format<km::HexDump>;

using namespace stdx::literals;

/// @brief Is the given byte printable and not a control character.
static bool IsText(uint8_t c) {
    return c >= 0x20 && c <= 0x7E;
}

static void FormatRow(km::IOutStream& out, std::span<const std::byte> row) {
    uintptr_t address = (uintptr_t)row.data();

    out.write(km::format(km::Hex(address).pad(16, '0')));
    out.write(" : "_sv);

    for (size_t i = 0; i < 16; i++) {
        if (i >= row.size()) {
            out.write("  "_sv);
        } else {
            out.write(km::FormatInt((uint8_t)row[i], 16));
        }

        if (i % 2 == 1) {
            out.write(" "_sv);
        }
    }

    for (size_t i = 0; i < 16; i++) {
        if (i >= row.size()) {
            out.write(" "_sv);
        } else {
            uint8_t c = (uint8_t)row[i];
            out.write(IsText(c) ? c : '.');
        }
    }
}

void HexDumpFormat::format(IOutStream &out, HexDump value) {
    size_t size = value.data.size();
    while (size > 16) {
        FormatRow(out, value.data.subspan(0, 16));
        out.write("\n"_sv);

        value.data = value.data.subspan(16);
        size -= 16;
    }

    FormatRow(out, value.data);
}
