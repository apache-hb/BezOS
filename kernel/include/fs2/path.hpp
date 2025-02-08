#pragma once

#include "shared/syscall.h"

#include "std/string.hpp"

namespace vfs2 {
    class VfsPath;
    class VfsPathConstIterator;

    using VfsString = stdx::StringBase<OsUtf8Char>;
    using VfsStringView = stdx::StringViewBase<OsUtf8Char>;

    static constexpr OsUtf8Char kPathSeparator = OS_PATH_SEPARATOR;
    static constexpr OsUtf8Char kPathInvalidChars[] = { OS_PATH_SEPARATOR, '/', '\\' };

    class VfsPathConstIterator {
        using Iterator = VfsString::const_iterator;

        const VfsString *mString;
        Iterator mIter;

        Iterator nextSegment(Iterator front) const;
        Iterator prevSegment(Iterator back) const;

        VfsStringView segment() const;

    public:
        VfsPathConstIterator(const VfsString *string, Iterator iter);

        VfsStringView operator*() const;

        VfsPathConstIterator& operator++();

        bool operator==(const VfsPathConstIterator& other) const;
    };

    class VfsPath {
        VfsString mPath;

    public:
        VfsPath() = default;

        VfsPath(VfsString path);

        template<size_t N> requires (N > 0)
        VfsPath(const char (&path)[N])
            : VfsPath(VfsString(std::begin(path), std::end(path) - 1))
        { }

        size_t count() const { return mPath.count(); }
        size_t segmentCount() const;

        VfsPathConstIterator begin() const;
        VfsPathConstIterator end() const;

        VfsStringView string() const { return mPath; }

        /// @brief The parent path for this path.
        ///
        /// @pre @a this->segmentCount() > 0
        VfsPath parent() const;

        /// @brief The name of the current file or folder, including extensions.
        VfsStringView name() const;

        friend constexpr auto operator<=>(const VfsPath& lhs, const VfsPath& rhs) {
            return lhs.mPath <=> rhs.mPath;
        }

        friend constexpr bool operator==(const VfsPath& lhs, const VfsPath& rhs) {
            return lhs.mPath == rhs.mPath;
        }
    };

    namespace detail {
        template<typename... Args>
        VfsString BuildPathText(Args&&... args) {
            VfsString path;

            auto addSegment = [&](auto&& segment) {
                if (!path.isEmpty()) {
                    path.add(kPathSeparator);
                }

                path.append(segment);
            };

            (addSegment(std::forward<Args>(args)), ...);

            return path;
        }
    }

    template<typename... Args>
    VfsPath BuildPath(Args&&... args) {
        return VfsPath(detail::BuildPathText(std::forward<Args>(args)...));
    }

    bool VerifyPathText(VfsStringView text);
}
