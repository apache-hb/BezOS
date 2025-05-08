#include "fs/path.hpp"

#include <algorithm>
#include <cassert>

using namespace vfs;

VfsPathConstIterator::Iterator VfsPathConstIterator::nextSegment(Iterator front) const {
    auto end = mString->end();

    auto iter = front;
    while (iter != end && *iter != OS_PATH_SEPARATOR) {
        iter++;
    }

    return iter;
}

VfsStringView VfsPathConstIterator::segment() const {
    auto back = nextSegment(mIter);
    return VfsStringView(mIter, back);
}

VfsPathConstIterator::VfsPathConstIterator(const VfsString *string, Iterator iter)
    : mString(string)
    , mIter(iter)
{ }

VfsStringView VfsPathConstIterator::operator*() const {
    return segment();
}

VfsPathConstIterator& VfsPathConstIterator::operator++() {
    auto back = nextSegment(mIter);
    if (back != mString->end()) {
        back++;
    }

    mIter = back;

    return *this;
}

bool VfsPathConstIterator::operator==(const VfsPathConstIterator& other) const {
    return mIter == other.mIter;
}

VfsPath::VfsPath(stdx::String path)
    : mPath(std::move(path))
{ }

VfsPathConstIterator VfsPath::begin() const {
    return VfsPathConstIterator(&mPath, mPath.begin());
}

VfsPathConstIterator VfsPath::end() const {
    return VfsPathConstIterator(&mPath, mPath.end());
}

size_t VfsPath::segmentCount() const {
    //
    // The root path is represented by the empty string.
    //
    if (mPath.isEmpty()) return 0;

    return std::ranges::count(mPath, OS_PATH_SEPARATOR) + 1;
}

VfsPath VfsPath::parent() const {
    auto tail = std::ranges::find_last(mPath, OS_PATH_SEPARATOR);

    return VfsPath(VfsString(mPath.begin(), tail.begin()));
}

VfsStringView VfsPath::name() const {
    auto tail = std::ranges::find_last(mPath, OS_PATH_SEPARATOR);

    if (tail.empty()) {
        return mPath;
    }

    return VfsStringView(tail.begin() + 1, mPath.end());
}

bool vfs::VerifyPathText(VfsStringView text) {
    //
    // Empty paths are the root path.
    //
    if (text.isEmpty()) {
        return true;
    }

    //
    // Paths cannot start or end with a separator.
    //
    if (text.front() == OS_PATH_SEPARATOR || text.back() == OS_PATH_SEPARATOR) {
        return false;
    }

    //
    // Paths cannot contain any empty segments.
    //
    for (size_t i = 0; i < text.count() - 1; i++) {
        if (text[i] == OS_PATH_SEPARATOR && text[i + 1] == OS_PATH_SEPARATOR) {
            return false;
        }
    }

    //
    // Nodes cannot be named '.', We require the userspace
    // to normalize paths before passing them to the kernel.
    //
    if (text.startsWith(".\0") || text.endsWith("\0.")) {
        return false;
    }

    if (std::ranges::contains_subrange(text, stdx::StringView("\0.\0"))) {
        return false;
    }

    //
    // Paths cannot contain any invalid characters.
    //
    for (size_t i = 0; i < text.count(); i++) {
        if (text[i] == OS_PATH_SEPARATOR) {
            continue;
        }

        if (std::ranges::contains(kPathInvalidChars, text[i])) {
            return false;
        }
    }

    return true;
}

using PathFormat = km::Format<VfsPath>;

void PathFormat::format(km::IOutStream& out, const vfs::VfsPath& path) {
    for (VfsStringView segment : path) {
        out.format("/", segment);
    }
}
