#include "fs/path.hpp"

stdx::StringView km::VfsPathIterator::segment() const {
    stdx::StringView path = mPath->path();
    auto begin = path.begin() + mOffset;
    auto end = path.end();

    auto it = std::find(begin + 1, end, VfsPath::kSeperator);

    return stdx::StringView(begin + 1, it);
}

bool km::VfsPathIterator::operator!=(const VfsPathIterator& other) const {
    return mSegment != other.mSegment;
}

km::VfsPathIterator& km::VfsPathIterator::operator++() {
    mSegment += 1;
    mOffset += segment().count() + 1;
    return *this;
}

stdx::StringView km::VfsPathIterator::operator*() const {
    return segment();
}

km::VfsPathIterator km::VfsPath::begin() const {
    return VfsPathIterator(this, 0, 0);
}

km::VfsPathIterator km::VfsPath::end() const {
    return VfsPathIterator(this, mSegments, 0);
}

km::VfsPath km::VfsPath::parent() const {
    auto back = mPath.end();
    while (*back != kSeperator) {
        back--;
    }

    if (back == mPath.begin()) {
        return VfsPath(stdx::String("/"));
    }

    return VfsPath(stdx::StringView(mPath.begin(), back));
}

stdx::StringView km::VfsPath::name() const {
    auto back = mPath.end();
    auto front = mPath.end();
    while (*front != kSeperator) {
        front--;
    }

    return stdx::StringView(front + 1, back);
}
