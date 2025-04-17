#include <sanitizer/ubsan_interface.h>

#include "arch/abi.hpp"
#include "kernel.hpp"
#include "log.hpp"
#include "panic.hpp"
#include "util/format.hpp"

/// LLVM Project begin - SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
/// Situations in which we might emit a check for the suitability of a
/// pointer or glvalue. Needs to be kept in sync with CodeGenFunction.h in
/// clang.
enum TypeCheckKind : unsigned char {
    /// Checking the operand of a load. Must be suitably sized and aligned.
    TCK_Load,
    /// Checking the destination of a store. Must be suitably sized and aligned.
    TCK_Store,
    /// Checking the bound value in a reference binding. Must be suitably sized
    /// and aligned, but is not required to refer to an object (until the
    /// reference is used), per core issue 453.
    TCK_ReferenceBinding,
    /// Checking the object expression in a non-static data member access. Must
    /// be an object within its lifetime.
    TCK_MemberAccess,
    /// Checking the 'this' pointer for a call to a non-static member function.
    /// Must be an object within its lifetime.
    TCK_MemberCall,
    /// Checking the 'this' pointer for a constructor call.
    TCK_ConstructorCall,
    /// Checking the operand of a static_cast to a derived pointer type. Must be
    /// null or an object within its lifetime.
    TCK_DowncastPointer,
    /// Checking the operand of a static_cast to a derived reference type. Must
    /// be an object within its lifetime.
    TCK_DowncastReference,
    /// Checking the operand of a cast to a base object. Must be suitably sized
    /// and aligned.
    TCK_Upcast,
    /// Checking the operand of a cast to a virtual base object. Must be an
    /// object within its lifetime.
    TCK_UpcastToVirtualBase,
    /// Checking the value assigned to a _Nonnull pointer. Must not be null.
    TCK_NonnullAssign,
    /// Checking the operand of a dynamic_cast or a typeid expression.  Must be
    /// null or an object within its lifetime.
    TCK_DynamicOperation
};
/// LLVM Project end - SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

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
    UbsanSourceLocation attrLocation;
    int argIndex;
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
    TypeCheckKind typeCheckKind;
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

struct UbsanImplicitConversionData {
    UbsanSourceLocation location;
    UbsanTypeDescriptor *fromType;
    UbsanTypeDescriptor *toType;
    uint8_t kind;
};

template<>
struct km::Format<TypeCheckKind> {
    static void format(km::IOutStream& out, TypeCheckKind kind) {
        switch (kind) {
        case TCK_Load:
            out.format("Load");
            break;
        case TCK_Store:
            out.format("Store");
            break;
        case TCK_ReferenceBinding:
            out.format("ReferenceBinding");
            break;
        case TCK_MemberAccess:
            out.format("MemberAccess");
            break;
        case TCK_MemberCall:
            out.format("MemberCall");
            break;
        case TCK_ConstructorCall:
            out.format("ConstructorCall");
            break;
        case TCK_DowncastPointer:
            out.format("DowncastPointer");
            break;
        case TCK_DowncastReference:
            out.format("DowncastReference");
            break;
        case TCK_Upcast:
            out.format("Upcast");
            break;
        case TCK_UpcastToVirtualBase:
            out.format("UpcastToVirtualBase");
            break;
        case TCK_NonnullAssign:
            out.format("NonnullAssign");
            break;
        case TCK_DynamicOperation:
            out.format("DynamicOperation");
            break;
        default:
            out.format("Unknown(", (int)kind, ")");
            break;
        }
    }
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

static constexpr bool kEmitAddrToLine = true;
static constexpr stdx::StringView kImagePath = "install/kernel/bin/bezos-limine.elf";

#define UBSAN_API [[gnu::section(".text.ubsan"), gnu::no_sanitize("undefined")]]

[[noreturn]] UBSAN_API
static void UbsanReportError(stdx::StringView message) {
    if (km::SystemMemory *memory = km::GetSystemMemory()) {
        void *rbp = __builtin_frame_address(0);
        x64::WalkStackFramesChecked(memory->systemTables(), (void**)rbp, [](void **frame, void *pc, stdx::StringView note) {
            KmDebugMessageUnlocked("| ", (void*)frame, " | ", pc);
            if (!note.isEmpty()) {
                KmDebugMessageUnlocked(" ", note);
            }
            KmDebugMessageUnlocked("\n");
        });


        if (kEmitAddrToLine) {
            KmDebugMessageUnlocked("llvm-addr2line -e ", kImagePath);

            x64::WalkStackFramesChecked(memory->systemTables(), (void**)rbp, [](void **, void *pc, stdx::StringView) {
                KmDebugMessageUnlocked(" ", pc);
            });

            KmDebugMessageUnlocked("\n");
        }
    }

    KM_PANIC(message);
}

extern "C" UBSAN_API void __ubsan_handle_add_overflow(UbsanOverflowData *data, uintptr_t, uintptr_t) {
    KmDebugMessage("[UBSAN] Add overflow: ", data->location, "\n");
    UbsanReportError("UBSAN Add Overflow");
}

extern "C" UBSAN_API void __ubsan_handle_sub_overflow(UbsanOverflowData *data, uintptr_t, uintptr_t) {
    KmDebugMessage("[UBSAN] Sub overflow: ", data->location, "\n");
    UbsanReportError("UBSAN Sub Overflow");
}

extern "C" UBSAN_API void __ubsan_handle_mul_overflow(UbsanOverflowData *data, uintptr_t, uintptr_t) {
    KmDebugMessage("[UBSAN] Mul overflow: ", data->location, "\n");
    UbsanReportError("UBSAN Mul Overflow");
}

extern "C" UBSAN_API void __ubsan_handle_divrem_overflow(UbsanOverflowData *data, uintptr_t, uintptr_t) {
    KmDebugMessage("[UBSAN] Divrem overflow: ", data->location, "\n");
    UbsanReportError("UBSAN Divrem Overflow");
}

extern "C" UBSAN_API void __ubsan_handle_negate_overflow(UbsanOverflowData *data, uintptr_t) {
    KmDebugMessage("[UBSAN] Negate overflow: ", data->location, "\n");
    UbsanReportError("UBSAN Negate Overflow");
}

extern "C" UBSAN_API void __ubsan_handle_pointer_overflow(UbsanOverflowData *data, uintptr_t, uintptr_t) {
    KmDebugMessage("[UBSAN] Pointer overflow: ", data->location, "\n");
    UbsanReportError("UBSAN Pointer Overflow");
}

extern "C" UBSAN_API void __ubsan_handle_builtin_unreachable(UbsanUnreachableData *data) {
    KmDebugMessage("[UBSAN] Unreachable: ", data->location, "\n");
    UbsanReportError("UBSAN Unreachable");
}

extern "C" UBSAN_API void __ubsan_handle_missing_return(UbsanUnreachableData *data) {
    KmDebugMessage("[UBSAN] Missing return: ", data->location, "\n");
    UbsanReportError("UBSAN Missing Return");
}

extern "C" UBSAN_API void __ubsan_handle_nonnull_arg(UbsanNonnullArgData *data) {
    KmDebugMessage("[UBSAN] Nonnull arg: ", data->location, "\n");
    UbsanReportError("UBSAN Nonnull Arg");
}

extern "C" UBSAN_API void __ubsan_handle_load_invalid_value(UbsanInvalidValueData *data, uintptr_t address) {
    KmDebugMessage("[UBSAN] Load invalid value: ", data->location, "\n");
    KmDebugMessage("[UBSAN] Type: ", *data->type, ", Address: ", (void*)address, "\n");
    UbsanReportError("UBSAN Load Invalid Value");
}

extern "C" UBSAN_API void __ubsan_handle_nonnull_return(UbsanNonnullReturnData *data) {
    KmDebugMessage("[UBSAN] Nonnull return: ", data->location, "\n");
    UbsanReportError("UBSAN Nonnull Return");
}

extern "C" UBSAN_API void __ubsan_handle_out_of_bounds(UbsanOutOfBoundsData *data, uintptr_t, uintptr_t) {
    KmDebugMessage("[UBSAN] Out of bounds: ", data->location, "\n");
    UbsanReportError("UBSAN Out of Bounds");
}

extern "C" UBSAN_API void __ubsan_handle_invalid_builtin(UbsanInvalidBuiltinData *data, uintptr_t) {
    KmDebugMessage("[UBSAN] Invalid builtin: ", data->location, "\n");
    UbsanReportError("UBSAN Invalid Builtin");
}

extern "C" UBSAN_API void __ubsan_handle_type_mismatch_v1(UbsanTypeMismatchDataV1 *data, uintptr_t address) {
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

extern "C" UBSAN_API void __ubsan_handle_nonnull_return_v1(UbsanNonnullReturnData *data, UbsanSourceLocation *) {
    KmDebugMessage("[UBSAN] Nonnull return: ", data->location, "\n");
    UbsanReportError("UBSAN Nonnull Return");
}

extern "C" UBSAN_API void __ubsan_handle_alignment_assumption(UbsanAlignmentAssumptionData *data, uintptr_t, uintptr_t) {
    KmDebugMessage("[UBSAN] Alignment assumption: ", data->location, "\n");
    UbsanReportError("UBSAN Alignment Assumption");
}

extern "C" UBSAN_API void __ubsan_handle_shift_out_of_bounds(UbsanShiftOutOfBoundsData *data, uintptr_t lhs, uintptr_t rhs) {
    KmDebugMessage("[UBSAN] Shift out of bounds: ", data->location, "\n");
    KmDebugMessage("[UBSAN] Shift ", lhs, " by ", rhs, "\n");
    UbsanReportError("UBSAN Shift Out of Bounds");
}

extern "C" UBSAN_API void __ubsan_handle_implicit_conversion(UbsanImplicitConversionData *data, uintptr_t lhs, uintptr_t rhs) {
    KmDebugMessage("[UBSAN] Implicit conversion: ", data->location, "\n");
    KmDebugMessage("[UBSAN] From: ", *data->fromType, ", To: ", *data->toType, "\n");
    KmDebugMessage("[UBSAN] LHS: ", (void*)lhs, ", RHS: ", (void*)rhs, "\n");
    UbsanReportError("UBSAN Implicit Conversion");
}

extern "C" UBSAN_API void __ubsan_handle_nullability_arg(UbsanNonnullArgData *data, uintptr_t) {
    KmDebugMessage("[UBSAN] Nullability arg: ", data->location, "\n");
    UbsanReportError("UBSAN Nullability Arg");
}
