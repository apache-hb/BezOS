#pragma once

#include <string>

namespace os {
    class path {
        std::string m_path;

    public:
        path() = default;

        path(std::string segment)
            : m_path(std::move(segment))
        { }

        size_t size() const noexcept { return m_path.size(); }

        size_t segment_count() const noexcept;
    };
}
