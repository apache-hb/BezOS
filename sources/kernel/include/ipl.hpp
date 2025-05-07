#pragma once

#include "util/util.hpp"

#include "common/compiler/compiler.hpp"

namespace km {
    /// @brief Interrupt Priority Level
    enum class Ipl {
        ePassive,
        eDispatch,
    };

    namespace detail {
        /// @brief Assert that the current IPL is at least the given level.
        /// Aborts if the current IPL is lower than the given level.
        /// @param level The level to assert.
        void EnforceIpl(Ipl level) noexcept;

        /// @brief Raise the IPL from the given level to the target level.
        /// Aborts if the current IPL is not @p from.
        /// @param from The level to raise from.
        /// @param to The level to raise to.
        void RaiseIpl(Ipl from, Ipl to) noexcept;

        /// @brief Lower the IPL from the given level to the target level.
        /// Aborts if the current IPL is not @p from.
        /// @param from The level to lower from.
        /// @param to The level to lower to.
        void LowerIpl(Ipl from, Ipl to) noexcept;
    }

    template<Ipl Level>
    class IplTag;

    template<Ipl Level>
    IplTag<Level> EnforceIpl();

    template<Ipl To, Ipl From>
    IplTag<To> RaiseIpl(IplTag<From>&& from);

    template<Ipl To, Ipl From>
    IplTag<To> LowerIpl(IplTag<From>&& from);

    template<Ipl Level>
    void ReleaseIpl(IplTag<Level>&& tag) noexcept [[clang::nonblocking]];

    template<Ipl Level>
    class [[clang::consumable(unconsumed), nodiscard]] IplTag {
        friend IplTag<Level> EnforceIpl<Level>();

        template<Ipl To, Ipl From>
        friend IplTag<To> RaiseIpl(IplTag<From>&& from);

        template<Ipl To, Ipl From>
        friend IplTag<To> LowerIpl(IplTag<From>&& from);

        friend void ReleaseIpl<Level>(IplTag<Level>&& tag) noexcept [[clang::nonblocking]];

        [[clang::return_typestate(unconsumed)]]
        constexpr IplTag(sm::init) noexcept {}

        [[clang::return_typestate(unconsumed)]]
        static IplTag unchecked() noexcept {
            return IplTag{sm::init{}};
        }
    public:
        constexpr IplTag() noexcept = delete;
        constexpr IplTag(const IplTag&) = delete;
        constexpr IplTag& operator=(const IplTag&) = delete;
        constexpr IplTag& operator=(IplTag&&) = delete;

        constexpr IplTag(IplTag&& other [[clang::return_typestate(consumed)]]) noexcept = default;

        /// @note Destructor is non-trivial to workaround clang bug https://github.com/llvm/llvm-project/issues/138890
        [[clang::callable_when(consumed)]]
        constexpr ~IplTag() noexcept { }

        template<Ipl Next> requires (Level >= Next)
        [[clang::callable_when(unconsumed)]]
        operator IplTag<Next>() const {
            return IplTag<Next>{};
        }

        [[clang::callable_when(unconsumed)]]
        Ipl level() const noexcept { return Level; }
    };

    template<Ipl Level>
    IplTag<Level> EnforceIpl() {
        detail::EnforceIpl(Level);
        return IplTag<Level>::unchecked();
    }

    template<Ipl To, Ipl From>
    IplTag<To> RaiseIpl(IplTag<From>&&) {
        detail::RaiseIpl(From, To);
        return IplTag<To>::unchecked();
    }

    template<Ipl To, Ipl From>
    IplTag<To> LowerIpl(IplTag<From>&&) {
        detail::LowerIpl(From, To);
        return IplTag<To>::unchecked();
    }

    template<Ipl Level>
    void ReleaseIpl(IplTag<Level>&& tag) noexcept [[clang::nonblocking]] {
        CLANG_DIAGNOSTIC_PUSH();
        CLANG_DIAGNOSTIC_IGNORE("-Wconsumed");
        CLANG_DIAGNOSTIC_IGNORE("-Wunused-result");

        std::move(tag);

        CLANG_DIAGNOSTIC_POP();
    }
}
