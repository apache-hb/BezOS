#pragma once

#include "panic.hpp"

#include "std/string.hpp"
#include "std/string_view.hpp"

#include <utility>

namespace km {
    class VfsEntry;
    class VfsPath;

    class VfsPathIterator {
        const VfsPath *mPath;
        uint16_t mSegment;
        uint16_t mOffset;

        stdx::StringView segment() const;

    public:
        VfsPathIterator(const VfsPath *path, uint16_t segment, uint16_t offset)
            : mPath(path)
            , mSegment(segment)
            , mOffset(offset)
        { }

        bool operator!=(const VfsPathIterator& other) const;

        VfsPathIterator& operator++();
        stdx::StringView operator*() const;
    };

    class VfsPath {
        stdx::String mPath;
        uint16_t mSegments;

    public:
        static constexpr char kSeperator = '/';

        VfsPath() = default;

        VfsPath(stdx::StringView path)
            : VfsPath(stdx::String(path))
        { }

        VfsPath(stdx::String path)
            : mPath(std::move(path))
            , mSegments(std::count_if(mPath.begin(), mPath.end(), [](char c) { return c == kSeperator; }))
        {
            KM_CHECK(!mPath.isEmpty(), "Path cannot be empty.");
            KM_CHECK(mPath.front() == kSeperator, "Path must start with '/'.");

            if (mPath.count() > 1)
                KM_CHECK(mPath.back() != kSeperator, "Path must not end with '/'.");
        }

        stdx::StringView path() const { return mPath; }
        uint16_t segments() const { return mSegments; }

        VfsPathIterator begin() const;
        VfsPathIterator end() const;

        VfsPath parent() const;
        stdx::StringView name() const;

        constexpr auto operator<=>(const VfsPath& other) const {
            return mPath <=> other.mPath;
        }
    };
}
