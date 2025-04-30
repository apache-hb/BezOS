#include "util/format.hpp"

using HexDumpFormat = km::Format<km::HexDump>;
using OsStatusFormat = km::Format<OsStatusId>;

using namespace stdx::literals;

/// @brief Is the given byte printable and not a control character.
static bool IsText(uint8_t c) {
    return c >= 0x20 && c <= 0x7E;
}

static constexpr size_t kRowSize = 16;

static void FormatRow(km::IOutStream& out, std::span<const std::byte> row, uintptr_t address) {
    out.write(km::format(km::Hex(address).pad(16, '0')));
    out.write(" : ");

    for (size_t i = 0; i < kRowSize; i++) {
        if (i >= row.size()) {
            out.write("  ");
        } else {
            out.write(km::FormatInt((uint8_t)row[i], 16, 2, '0'));
        }

        if (i % 2 == 1) {
            out.write(" ");
        }
    }

    out.write("  ");

    for (size_t i = 0; i < kRowSize; i++) {
        if (i >= row.size()) {
            out.write(" ");
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
        FormatRow(out, value.data.subspan(start, kRowSize), value.base + start);
        out.write("\n");
        start += kRowSize;
    }

    FormatRow(out, value.data.subspan(start), value.base + start);
}

void OsStatusFormat::format(IOutStream& out, OsStatusId value) {
    auto result = [&](stdx::StringView message) {
        out.format(message, " (", km::Hex(OsStatus(value)).pad(8, '0'), ")");
    };
    switch (value) {
    case OsStatusSuccess:
        result("Success");
        break;
    case OsStatusOutOfMemory:
        result("Out of memory");
        break;
    case OsStatusNotFound:
        result("Not found");
        break;
    case OsStatusInvalidInput:
        result("Invalid input");
        break;
    case OsStatusNotSupported:
        result("Not supported");
        break;
    case OsStatusAlreadyExists:
        result("Already exists");
        break;
    case OsStatusTraverseNonFolder:
        result("Traverse non-folder");
        break;
    case OsStatusInvalidType:
        result("Invalid type");
        break;
    case OsStatusHandleLocked:
        result("Handle locked");
        break;
    case OsStatusInvalidPath:
        result("Invalid path");
        break;
    case OsStatusInvalidFunction:
        result("Invalid function");
        break;
    case OsStatusEndOfFile:
        result("End of file");
        break;
    case OsStatusInvalidData:
        result("Invalid data");
        break;
    case OsStatusInvalidVersion:
        result("Invalid version");
        break;
    case OsStatusTimeout:
        result("Timeout");
        break;
    case OsStatusOutOfBounds:
        result("Out of bounds");
        break;
    case OsStatusMoreData:
        result("More data");
        break;
    case OsStatusChecksumError:
        result("Checksum error");
        break;
    case OsStatusInvalidHandle:
        result("Invalid handle");
        break;
    case OsStatusInvalidAddress:
        result("Invalid address");
        break;
    case OsStatusInvalidSpan:
        result("Invalid span");
        break;
    case OsStatusDeviceFault:
        result("Device fault");
        break;
    case OsStatusDeviceBusy:
        result("Device busy");
        break;
    case OsStatusDeviceNotReady:
        result("Device not ready");
        break;
    case OsStatusInterfaceNotSupported:
        result("Interface not supported");
        break;
    case OsStatusFunctionNotSupported:
        result("Function not supported");
        break;
    case OsStatusOperationNotSupported:
        result("Operation not supported");
        break;
    case OsStatusCompleted:
        result("Completed");
        break;
    case OsStatusAccessDenied:
        result("Access denied");
        break;
    case OsStatusProcessOrphaned:
        result("Process orphaned");
        break;
    case OsStatusNotAvailable:
        result("Not available");
        break;
    default:
        result("Unknown");
        break;
    }
}
