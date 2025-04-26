#include "memory/pt_command_list.hpp"
#include "memory/pte.hpp"
#include "panic.hpp"

using PtCommandList = km::PageTableCommandList;
using PtCommand = km::detail::PageTableCommand;

static OsStatus ValidateCommandRange(km::VirtualRange range) noexcept [[clang::nonblocking]] {
    // All commands must operate on valid ranges of virtual memory.
    if (range.isEmpty() || !range.isValid()) {
        return OsStatusInvalidSpan;
    }

    // All commands must operate on aligned ranges of virtual memory.
    if (range != km::alignedOut(range, x64::kPageSize)) {
        return OsStatusInvalidInput;
    }

    return OsStatusSuccess;
}

OsStatus PtCommandList::add(PtCommand command) noexcept [[clang::allocating]] {
    // The command list must have extra space reserved by the caller.
    KM_CHECK(mCommands.capacity() > mCommands.count(), "Command list is full");

    if (OsStatus status = mCommands.add(command)) {
        return status;
    }

    return OsStatusSuccess;
}

km::PageTableCommandList::~PageTableCommandList() noexcept [[clang::nonallocating]] {
    mTables->drainTableList(mStorage);
}

OsStatus PtCommandList::validate() const noexcept [[clang::nonallocating]] {
    for (size_t i = 0; i < mCommands.count(); i++) {
        const PtCommand &command = mCommands[i];

        for (size_t j = 0; j < mCommands.count(); j++) {
            if (i == j) {
                continue;
            }

            const PtCommand &other = mCommands[j];
            if (km::outerAdjacent(command.range, other.range)) {
                continue;
            }

            // All commands must operate on disjoint ranges of virtual memory.
            if (command.range.overlaps(other.range) || command.range.contains(other.range)) {
                return OsStatusInvalidData;
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
    if (OsStatus status = ValidateCommandRange(range)) [[unlikely]] {
        return status;
    }

    if (OsStatus status = mCommands.ensureExtra(1)) {
        return status;
    }

    detail::PageTableList buffer;
    if (OsStatus status = mTables->reservePageTablesForUnmapping(range, buffer)) {
        return status;
    }

    PtCommand command {
        .command = PtCommand::eUnmap,
        .range = range,
    };

    if (OsStatus status = add(command)) {
        mTables->drainTableList(buffer);
        return status;
    }

    mStorage.append(buffer);

    return OsStatusSuccess;
}

OsStatus PtCommandList::map(AddressMapping mapping, PageFlags flags, MemoryType type) noexcept [[clang::allocating]] {
    if (OsStatus status = ValidateCommandRange(mapping.virtualRange())) [[unlikely]] {
        return status;
    }

    if (!mTables->verifyMapping(mapping)) [[unlikely]] {
        return OsStatusInvalidInput;
    }

    if (OsStatus status = mCommands.ensureExtra(1)) [[unlikely]] {
        return status;
    }

    detail::PageTableList buffer;
    if (OsStatus status = mTables->reservePageTablesForMapping(mapping.virtualRange(), buffer)) [[unlikely]] {
        return status;
    }

    PtCommand command {
        .command = PtCommand::eMap,
        .range = mapping.virtualRange(),
        .base = mapping.paddr,
        .flags = flags,
        .type = type,
    };

    if (OsStatus status = add(command)) {
        mTables->drainTableList(buffer);
        return status;
    }

    mStorage.append(buffer);

    return OsStatusSuccess;
}

km::PageTableCommandListStats PtCommandList::stats() const noexcept [[clang::nonallocating]] {
    size_t commandCount = mCommands.count();
    size_t tableCount = mStorage.count();
    return km::PageTableCommandListStats {
        .commandCount = commandCount,
        .tableCount = tableCount,
    };
}
