#pragma once

enum class DeferTag { eTag };
enum class ErrorDeferTag { eTag };

template<typename F>
constexpr auto operator+(DeferTag, F fn) {
    struct Inner {
        F fn;
        ~Inner() noexcept { fn(); }
    };

    return Inner{fn};
}

#define CONCAT2(a, b) a##b
#define CONCAT(a, b) CONCAT2(a, b)
#define EXPAND(x) x

#define defer const auto CONCAT(defer, __COUNTER__) = DeferTag::eTag + [&] ()
