#include "memory/heap_command_list.hpp"
#include "memory/heap.hpp"

using km::TlsfHeapCommandListStats;
using km::TlsfHeapCommandList;

void TlsfHeapCommandList::add(detail::TlsfHeapCommand command) noexcept [[clang::allocating]] {
    KM_ASSERT(mCommands.capacity() > mCommands.count());
    OsStatus status = mCommands.add(std::move(command));
    KM_ASSERT(status == OsStatusSuccess);
}

TlsfHeapCommandList::~TlsfHeapCommandList() noexcept [[clang::nonallocating]] {
    mHeap->drainBlockList(std::move(mStorage));
}

TlsfHeapCommandList::TlsfHeapCommandList(TlsfHeapCommandList&& other) noexcept [[clang::nonallocating]]
    : mHeap(other.mHeap)
    , mStorage(std::exchange(other.mStorage, nullptr))
    , mCommands(std::move(other.mCommands))
{ }

TlsfHeapCommandList& TlsfHeapCommandList::operator=(TlsfHeapCommandList&& other) noexcept [[clang::nonallocating]] {
    mHeap = other.mHeap;
    mStorage = std::exchange(other.mStorage, nullptr);
    mCommands = std::move(other.mCommands);
    return *this;
}

OsStatus TlsfHeapCommandList::split(TlsfAllocation allocation, uintptr_t midpoint, TlsfAllocation *lo [[outparam]], TlsfAllocation *hi [[outparam]]) noexcept [[clang::allocating]] {
    if (OsStatus status = mCommands.ensureExtra(1)) {
        return status;
    }

    if (OsStatus status = mHeap->allocateSplitBlocks(mStorage)) {
        return status;
    }

    detail::TlsfHeapCommand command {
        .command = detail::TlsfHeapCommand::eSplit,
        .subject = allocation,
        .midpoints = { { midpoint } },
    };

    command.results.split = { lo, hi };

    add(std::move(command));

    return OsStatusSuccess;
}

OsStatus TlsfHeapCommandList::splitv(TlsfAllocation allocation, std::span<const PhysicalAddress> points, std::span<TlsfAllocation> results) noexcept [[clang::allocating]] {
    if (OsStatus status = mCommands.ensureExtra(1)) {
        return status;
    }

    sm::InlinedVector<PhysicalAddress, 4> midpoints;
    if (OsStatus status = midpoints.add(points.data(), points.data() + points.size())) {
        return status;
    }

    if (OsStatus status = mHeap->allocateSplitVectorBlocks(points.size(), mStorage)) {
        return status;
    }

    detail::TlsfHeapCommand command {
        .command = detail::TlsfHeapCommand::eSplitVector,
        .subject = allocation,
        .midpoints = std::move(midpoints),
    };

    command.results.results = results;

    add(std::move(command));

    return OsStatusSuccess;
}

void TlsfHeapCommandList::commit() noexcept [[clang::nonallocating]] {
    auto commitSplit = [&](const detail::TlsfHeapCommand &command) {
        mHeap->commitSplitBlock(command.subject, command.midpoints[0], command.results.split[0], command.results.split[1], mStorage);
    };

    auto commitSplitVector = [&](const detail::TlsfHeapCommand &command) {
        mHeap->commitSplitVectorBlocks(command.subject, command.midpoints, command.results.results, mStorage);
    };

    for (const detail::TlsfHeapCommand &command : mCommands) {
        if (command.isSplitVector()) {
            commitSplitVector(command);
        } else {
            commitSplit(command);
        }
    }
}

OsStatus TlsfHeapCommandList::validate() const noexcept [[clang::nonallocating]] {
    for (size_t i = 0; i < mCommands.count(); i++) {
        for (size_t j = 0; j < mCommands.count(); j++) {
            if (i == j) {
                continue;
            }

            const detail::TlsfHeapCommand &lhs = mCommands[i];
            const detail::TlsfHeapCommand &rhs = mCommands[j];

            if (lhs.subject == rhs.subject) {
                return OsStatusInvalidData;
            }
        }
    }

    return OsStatusSuccess;
}

TlsfHeapCommandListStats TlsfHeapCommandList::stats() const noexcept [[clang::nonallocating]] {
    return TlsfHeapCommandListStats {
        .commandCount = mCommands.count(),
        .blockCount = mStorage.count(),
    };
}
