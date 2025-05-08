#include "ipl.hpp"
#include "panic.hpp"

#include "arch/intrin.hpp"

void km::detail::EnforceIpl(Ipl level) noexcept {
    uint64_t eflags = __builtin_ia32_readeflags_u64();

    switch (level) {
    case Ipl::ePassive:
        KM_CHECK((eflags & (1 << 9)) == 0, "Invalid IPL level");
        break;
    case Ipl::eDispatch:
        KM_CHECK((eflags & (1 << 9)) != 0, "Invalid IPL level");
        break;
    }
}

void km::detail::RaiseIpl(Ipl from, Ipl to) noexcept {
    KM_CHECK(from < to, "Invalid IPL level transition");
    EnforceIpl(from);
    arch::IntrinX86_64::cli();
}

void km::detail::LowerIpl(Ipl from, Ipl to) noexcept {
    KM_CHECK(from > to, "Invalid IPL level transition");
    EnforceIpl(from);
    arch::IntrinX86_64::sti();
}
