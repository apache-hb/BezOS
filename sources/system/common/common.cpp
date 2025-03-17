#include <sanitizer/ubsan_interface.h>

#include <bezos/facility/debug.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

struct UbsanFunctionTypeMismatchData {
    UbsanSourceLocation location;
    UbsanTypeDescriptor *type;
};

[[noreturn]]
static void UbsanError(const char *format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    OsDebugMessageInfo message {
        .Front = buffer,
        .Back = buffer + strnlen(buffer, sizeof(buffer)),
        .Info = eOsLogError,
    };
    OsDebugMessage(message);
    exit(1);
}

extern "C" void __ubsan_handle_add_overflow(UbsanOverflowData *data, uintptr_t, uintptr_t) {
    UbsanError("[UBSAN] Add overflow: %s:%u:%u\n", data->location.filename, data->location.line, data->location.column);
}

extern "C" void __ubsan_handle_sub_overflow(UbsanOverflowData *data, uintptr_t, uintptr_t) {
    UbsanError("[UBSAN] Sub overflow: %s:%u:%u\n", data->location.filename, data->location.line, data->location.column);
}

extern "C" void __ubsan_handle_mul_overflow(UbsanOverflowData *data, uintptr_t, uintptr_t) {
    UbsanError("[UBSAN] Mul overflow: %s:%u:%u\n", data->location.filename, data->location.line, data->location.column);
}

extern "C" void __ubsan_handle_divrem_overflow(UbsanOverflowData *data, uintptr_t, uintptr_t) {
    UbsanError("[UBSAN] Divrem overflow: %s:%u:%u\n", data->location.filename, data->location.line, data->location.column);
}

extern "C" void __ubsan_handle_negate_overflow(UbsanOverflowData *data, uintptr_t) {
    UbsanError("[UBSAN] Negate overflow: %s:%u:%u\n", data->location.filename, data->location.line, data->location.column);
}

extern "C" void __ubsan_handle_pointer_overflow(UbsanOverflowData *data, uintptr_t, uintptr_t) {
    UbsanError("[UBSAN] Pointer overflow: %s:%u:%u\n", data->location.filename, data->location.line, data->location.column);
}

extern "C" void __ubsan_handle_builtin_unreachable(UbsanUnreachableData *data) {
    UbsanError("[UBSAN] Unreachable: %s:%u:%u\n", data->location.filename, data->location.line, data->location.column);
}

extern "C" void __ubsan_handle_missing_return(UbsanUnreachableData *data) {
    UbsanError("[UBSAN] Missing return: %s:%u:%u\n", data->location.filename, data->location.line, data->location.column);
}

extern "C" void __ubsan_handle_nonnull_arg(UbsanNonnullArgData *data) {
    UbsanError("[UBSAN] Nonnull arg: %s:%u:%u\n", data->location.filename, data->location.line, data->location.column);
}

extern "C" void __ubsan_handle_load_invalid_value(UbsanInvalidValueData *data, uintptr_t address) {
    UbsanError("[UBSAN] Load invalid value: %s:%u:%u\n", data->location.filename, data->location.line, data->location.column);
}

extern "C" void __ubsan_handle_nonnull_return(UbsanNonnullReturnData *data) {
    UbsanError("[UBSAN] Nonnull return: %s:%u:%u\n", data->location.filename, data->location.line, data->location.column);
}

extern "C" void __ubsan_handle_out_of_bounds(UbsanOutOfBoundsData *data, uintptr_t, uintptr_t) {
    UbsanError("[UBSAN] Out of bounds: %s:%u:%u\n", data->location.filename, data->location.line, data->location.column);
}

extern "C" void __ubsan_handle_invalid_builtin(UbsanInvalidBuiltinData *data, uintptr_t) {
    UbsanError("[UBSAN] Invalid builtin: %s:%u:%u\n", data->location.filename, data->location.line, data->location.column);
}

extern "C" void __ubsan_handle_type_mismatch_v1(UbsanTypeMismatchDataV1 *data, uintptr_t address) {
    UbsanError("[UBSAN] Type mismatch: %s:%u:%u\n", data->location.filename, data->location.line, data->location.column);
}

extern "C" void __ubsan_handle_nonnull_return_v1(UbsanNonnullReturnData *data, UbsanSourceLocation *) {
    UbsanError("[UBSAN] Nonnull return v1: %s:%u:%u\n", data->location.filename, data->location.line, data->location.column);
}

extern "C" void __ubsan_handle_alignment_assumption(UbsanAlignmentAssumptionData *data, uintptr_t, uintptr_t) {
    UbsanError("[UBSAN] Alignment assumption: %s:%u:%u\n", data->location.filename, data->location.line, data->location.column);
}

extern "C" void __ubsan_handle_shift_out_of_bounds(UbsanShiftOutOfBoundsData *data, uintptr_t lhs, uintptr_t rhs) {
    UbsanError("[UBSAN] Shift out of bounds: %s:%u:%u\n", data->location.filename, data->location.line, data->location.column);
}

extern "C" void __ubsan_handle_function_type_mismatch(UbsanFunctionTypeMismatchData *data, uintptr_t) {
    UbsanError("[UBSAN] Function type mismatch: %s:%u:%u\n", data->location.filename, data->location.line, data->location.column);
}

extern "C" void __ubsan_handle_float_cast_overflow(void *data, uintptr_t) {
    UbsanError("[UBSAN] Float cast overflow\n");
}
