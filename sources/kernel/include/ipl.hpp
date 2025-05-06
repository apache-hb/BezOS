#pragma once

#include "util/util.hpp"
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
    class [[clang::consumable(unconsumed)]] IplTag {
        friend IplTag<Level> EnforceIpl<Level>();

        template<Ipl To, Ipl From>
        friend IplTag<To> RaiseIpl(IplTag<From>&& from);

        template<Ipl To, Ipl From>
        friend IplTag<To> LowerIpl(IplTag<From>&& from);

        constexpr IplTag(sm::init) noexcept {}

        static IplTag unchecked() noexcept {
            return IplTag{sm::init{}};
        }
    public:
        constexpr IplTag() noexcept = delete;
        constexpr IplTag(const IplTag&) = delete;
        constexpr IplTag& operator=(const IplTag&) = delete;
        constexpr IplTag& operator=(IplTag&&) = delete;

        [[clang::callable_when(unconsumed), clang::set_typestate(consumed)]]
        constexpr IplTag(IplTag&&) = default;

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
}
