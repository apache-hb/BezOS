#include <sanitizer/ubsan_interface.h>

#include "arch/abi.hpp"
#include "kernel.hpp"
#include "log.hpp"
#include "panic.hpp"
#include "util/format.hpp"

struct UbsanTypeDescriptor {
    uint16_t kind;
    uint16_t info;
    char name[];
};

struct UbsanSourceLocation {
    char *filename;
    uint32_t line;
    uint32_t column;
};

struct UbsanOverflowData {
    UbsanSourceLocation location;
    UbsanTypeDescriptor *type;
};

struct UbsanUnreachableData {
    UbsanSourceLocation location;
};

struct UbsanNonnullArgData {
    UbsanSourceLocation location;
};

struct UbsanInvalidValueData {
    UbsanSourceLocation location;
    UbsanTypeDescriptor *type;
};

struct UbsanNonnullReturnData {
    UbsanSourceLocation location;
    UbsanSourceLocation attrLocation;
};

struct UbsanOutOfBoundsData {
    UbsanSourceLocation location;
    UbsanTypeDescriptor *arrayType;
    UbsanTypeDescriptor *indexType;
};

struct UbsanInvalidBuiltinData {
    UbsanSourceLocation location;
    uint8_t kind;
};

struct UbsanTypeMismatchDataV1 {
    UbsanSourceLocation location;
    UbsanTypeDescriptor *type;
    unsigned char logAlignment;
    unsigned char typeCheckKind;
};

struct UbsanAlignmentAssumptionData {
    UbsanSourceLocation location;
    UbsanSourceLocation assumptionLocation;
    UbsanTypeDescriptor *type;
};

struct UbsanShiftOutOfBoundsData {
    UbsanSourceLocation location;
    UbsanTypeDescriptor *lhsType;
    UbsanTypeDescriptor *rhsType;
};

template<>
struct km::Format<UbsanSourceLocation> {
    static void format(km::IOutStream& out, const UbsanSourceLocation& loc) {
        stdx::StringView filename(loc.filename, loc.filename + std::char_traits<char>::length(loc.filename));
        out.format(filename, ":", loc.line, ":", loc.column);
    }
};

template<>
struct km::Format<UbsanTypeDescriptor> {
    static void format(km::IOutStream& out, const UbsanTypeDescriptor& desc) {
        stdx::StringView name(desc.name, std::char_traits<char>::length(desc.name));
        out.format(name, " (kind=", desc.kind, ", info=", desc.info, ")");
    }
};

[[noreturn]]
static void UbsanReportError(stdx::StringView message) {
    if (km::SystemMemory *memory = km::GetSystemMemory()) {
        void *rbp = __builtin_frame_address(0);
        x64::WalkStackFramesChecked(memory->pt, (void**)rbp, [](void **frame, void *pc, stdx::StringView note) {
            KmDebugMessageUnlocked("| ", (void*)frame, " | ", pc);
            if (!note.isEmpty()) {
                KmDebugMessageUnlocked(" ", note);
            }
            KmDebugMessageUnlocked("\n");
        });
    }

    KM_PANIC(message);
}

extern "C" void __ubsan_handle_add_overflow(UbsanOverflowData *data, uintptr_t, uintptr_t) {
    KmDebugMessage("[UBSAN] Add overflow: ", data->location, "\n");
    UbsanReportError("UBSAN Add Overflow");
}

extern "C" void __ubsan_handle_sub_overflow(UbsanOverflowData *data, uintptr_t, uintptr_t) {
    KmDebugMessage("[UBSAN] Sub overflow: ", data->location, "\n");
    UbsanReportError("UBSAN Sub Overflow");
}

extern "C" void __ubsan_handle_mul_overflow(UbsanOverflowData *data, uintptr_t, uintptr_t) {
    KmDebugMessage("[UBSAN] Mul overflow: ", data->location, "\n");
    UbsanReportError("UBSAN Mul Overflow");
}

extern "C" void __ubsan_handle_divrem_overflow(UbsanOverflowData *data, uintptr_t, uintptr_t) {
    KmDebugMessage("[UBSAN] Divrem overflow: ", data->location, "\n");
    UbsanReportError("UBSAN Divrem Overflow");
}

extern "C" void __ubsan_handle_negate_overflow(UbsanOverflowData *data, uintptr_t) {
    KmDebugMessage("[UBSAN] Negate overflow: ", data->location, "\n");
    UbsanReportError("UBSAN Negate Overflow");
}

extern "C" void __ubsan_handle_pointer_overflow(UbsanOverflowData *data, uintptr_t, uintptr_t) {
    KmDebugMessage("[UBSAN] Pointer overflow: ", data->location, "\n");
    UbsanReportError("UBSAN Pointer Overflow");
}

extern "C" void __ubsan_handle_builtin_unreachable(UbsanUnreachableData *data) {
    KmDebugMessage("[UBSAN] Unreachable: ", data->location, "\n");
    UbsanReportError("UBSAN Unreachable");
}

extern "C" void __ubsan_handle_missing_return(UbsanUnreachableData *data) {
    KmDebugMessage("[UBSAN] Missing return: ", data->location, "\n");
    UbsanReportError("UBSAN Missing Return");
}

extern "C" void __ubsan_handle_nonnull_arg(UbsanNonnullArgData *data) {
    KmDebugMessage("[UBSAN] Nonnull arg: ", data->location, "\n");
    UbsanReportError("UBSAN Nonnull Arg");
}

extern "C" void __ubsan_handle_load_invalid_value(UbsanInvalidValueData *data, uintptr_t) {
    KmDebugMessage("[UBSAN] Load invalid value: ", data->location, "\n");
    UbsanReportError("UBSAN Load Invalid Value");
}

extern "C" void __ubsan_handle_nonnull_return(UbsanNonnullReturnData *data) {
    KmDebugMessage("[UBSAN] Nonnull return: ", data->location, "\n");
    UbsanReportError("UBSAN Nonnull Return");
}

extern "C" void __ubsan_handle_out_of_bounds(UbsanOutOfBoundsData *data, uintptr_t, uintptr_t) {
    KmDebugMessage("[UBSAN] Out of bounds: ", data->location, "\n");
    UbsanReportError("UBSAN Out of Bounds");
}

extern "C" void __ubsan_handle_invalid_builtin(UbsanInvalidBuiltinData *data, uintptr_t) {
    KmDebugMessage("[UBSAN] Invalid builtin: ", data->location, "\n");
    UbsanReportError("UBSAN Invalid Builtin");
}

extern "C" void __ubsan_handle_type_mismatch_v1(UbsanTypeMismatchDataV1 *data, uintptr_t address) {
    stdx::StringView reason = "type mismatch";
    uintptr_t alignment = UINT64_C(1) << data->logAlignment;
    if (address == 0x0) {
        reason = "null pointer dereference";
    } else if (alignment && (address & (alignment - 1))) {
        reason = "misaligned access";
    }

    KmDebugMessage("[UBSAN] Type mismatch: ", data->location, " (", data->typeCheckKind, ")\n");
    KmDebugMessage("[UBSAN] Type: ", *data->type, ", Address: ", (void*)address, ", Reason: ", reason, "\n");
    KmDebugMessage("[UBSAN] Expected alignment: ", alignment, "\n");

    UbsanReportError("UBSAN Type Mismatch");
}

extern "C" void __ubsan_handle_nonnull_return_v1(UbsanNonnullReturnData *data, UbsanSourceLocation *) {
    KmDebugMessage("[UBSAN] Nonnull return: ", data->location, "\n");
    UbsanReportError("UBSAN Nonnull Return");
}

extern "C" void __ubsan_handle_alignment_assumption(UbsanAlignmentAssumptionData *data, uintptr_t, uintptr_t) {
    KmDebugMessage("[UBSAN] Alignment assumption: ", data->location, "\n");
    UbsanReportError("UBSAN Alignment Assumption");
}

extern "C" void __ubsan_handle_shift_out_of_bounds(UbsanShiftOutOfBoundsData *data, uintptr_t lhs, uintptr_t rhs) {
    KmDebugMessage("[UBSAN] Shift out of bounds: ", data->location, "\n");
    KmDebugMessage("[UBSAN] Shift ", lhs, " by ", rhs, "\n");
    UbsanReportError("UBSAN Shift Out of Bounds");
}
