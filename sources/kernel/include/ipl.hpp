#pragma once

namespace km {
    /// @brief Interrupt Priority Level
    enum class Ipl {
        ePassive,
        eDispatch,
    };

    template<Ipl Level>
    class IplTag;

    template<Ipl Level>
    IplTag<Level> EnforceIpl();

    template<Ipl From, Ipl To>
    IplTag<To> RaiseIpl(IplTag<From>&& from);

    template<Ipl From, Ipl To>
    IplTag<To> LowerIpl(IplTag<From>&& from);

    template<Ipl Level>
    class IplTag {
    public:
        constexpr IplTag() noexcept = delete;
        constexpr IplTag(const IplTag&) = delete;
        constexpr IplTag(IplTag&&) = default;
        constexpr IplTag& operator=(const IplTag&) = delete;
        constexpr IplTag& operator=(IplTag&&) = delete;

        template<Ipl Next> requires (Level >= Next)
        operator IplTag<Next>() const {
            return IplTag<Next>{};
        }

        Ipl level() const noexcept { return Level; }
    };
}
