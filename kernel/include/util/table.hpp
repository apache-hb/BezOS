#pragma once

#include "std/string_view.hpp"
#include "util/format.hpp"

#include <array>
#include <cstddef>
#include <tuple>

namespace sm {
    struct TableHeader {
        stdx::StringView header;
        size_t width;
    };

    template<typename T, typename F, std::size_t... I>
    void IterateTuple(T&& tuple, F&& func, std::index_sequence<I...>) {
        (func(I, std::get<I>(tuple)), ...);
    }

    template<size_t N>
    struct TableDisplay {
        std::array<TableHeader, N> headers;

        void header(km::IOutStream& out) const {
            out.write("|");
            for (size_t i = 0; i < N; i++) {
                const TableHeader& header = headers[i];
                out.format(" ", header.header);

                size_t padding = header.width - header.header.count();
                for (size_t j = 0; j < padding; j++) {
                    out.write(" ");
                }
            }

            out.write("\n");

            out.write("|");

            for (size_t i = 0; i < N; i++) {
                const TableHeader& header = headers[i];
                for (size_t j = 0; j < header.width; j++) {
                    out.write("-");
                }

                out.write("+");
            }

            out.write("\n");
        }

        template<typename... A> requires (sizeof...(A) == N)
        void row(km::IOutStream& out, A&&... args) const {
            out.write("|");

            IterateTuple(std::forward_as_tuple(args...), [&](size_t index, auto&& arg) {
                auto buffer = km::ToStaticString<64>(arg);
                out.format(" ", buffer);
                size_t padding = headers[index].width - buffer.count();
                for (size_t i = 0; i < padding; i++) {
                    out.write(" ");
                }
            }, std::make_index_sequence<N>());

            out.write("\n");
        }
    };
}
