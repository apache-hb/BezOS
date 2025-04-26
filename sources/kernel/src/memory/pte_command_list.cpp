#include "memory/pt_command_list.hpp"
#include "memory/pte.hpp"
#include "panic.hpp"

using PtCommandList = km::PageTableCommandList;
using PtCommand = km::detail::PageTableCommand;

OsStatus PtCommandList::add(PtCommand command) noexcept [[clang::allocating]] {
    if (OsStatus status = mCommands.add(command)) {
        return status;
    }

    if constexpr (kEnablePageTableCommandListHardening) {
        return validate();
    }

    return OsStatusSuccess;
}

km::PageTableCommandList::~PageTableCommandList() noexcept [[clang::nonallocating]] {
    mTables->drainTableList(mStorage);
}

OsStatus PtCommandList::validate() const noexcept [[clang::nonallocating]] {
    for (size_t i = 0; i < mCommands.count(); i++) {
        const PtCommand &command = mCommands[i];
        if (command.range.isEmpty()) {
            return OsStatusInvalidInput;
        }

        for (size_t j = 0; j < mCommands.count(); j++) {
            if (i == j) {
                continue;
            }

            // All commands must operate on disjoint ranges of virtual memory.
            const PtCommand &other = mCommands[j];
            if (command.range.overlaps(other.range) || command.range.contains(other.range)) {
                return OsStatusInvalidInput;
            }
        }
    }

    return OsStatusSuccess;
}

void PtCommandList::commit() noexcept [[clang::nonallocating]] {
    auto applyMapping = [&](const PtCommand &command) {
        AddressMapping mapping = MappingOf(command.range, command.base);
        mTables->mapWithList(mapping, command.flags, command.type, mStorage);
    };

    auto applyUnmapping = [&](const PtCommand &command) {
        mTables->unmapWithList(command.range, mStorage);
    };

    for (const PtCommand &command : mCommands) {
        switch (command.command) {
        case PtCommand::eMap:
            applyMapping(command);
            break;
        case PtCommand::eUnmap:
            applyUnmapping(command);
            break;
        default:
            KM_PANIC("Invalid command");
        }
    }
}

OsStatus PtCommandList::unmap(VirtualRange range) noexcept [[clang::allocating]] {
    if (OsStatus status = mCommands.ensureExtra(1)) {
        return status;
    }

    if (OsStatus status = mTables->reservePageTablesForUnmapping(range, mStorage)) {
        return status;
    }

    PtCommand command {
        .command = PtCommand::eUnmap,
        .range = range,
    };

    // This is known to succeed, the command list has been pre-allocated.
    OsStatus status = add(command);
    KM_ASSERT(status == OsStatusSuccess);

    return OsStatusSuccess;
}

OsStatus PtCommandList::map(AddressMapping mapping, PageFlags flags, MemoryType type) noexcept [[clang::allocating]] {
    if (!mTables->verifyMapping(mapping)) [[unlikely]] {
        return OsStatusInvalidInput;
    }

    if (OsStatus status = mCommands.ensureExtra(1)) [[unlikely]] {
        return status;
    }

    if (OsStatus status = mTables->reservePageTablesForMapping(mapping.virtualRange(), mStorage)) [[unlikely]] {
        return status;
    }

    PtCommand command {
        .command = PtCommand::eMap,
        .range = mapping.virtualRange(),
        .base = mapping.paddr,
        .flags = flags,
        .type = type,
    };

    // This is known to succeed, the command list has been pre-allocated.
    OsStatus status = add(command);
    KM_ASSERT(status == OsStatusSuccess);

    return OsStatusSuccess;
}
