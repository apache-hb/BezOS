#pragma once

#include "std/detail/counted.hpp"
namespace sm {
    template<typename T>
    class RcuShared {
        using Counted = sm::detail::CountedObject<T>;
        Counted *mControl { nullptr };

        template<typename U>
        friend class RcuAtomic;

        Counted *exchangeControl(Counted *other = nullptr) noexcept {
            return std::exchange(mControl, other);
        }

        RcuShared(Counted *control) noexcept
            : mControl(control)
        { }

    public:
        constexpr RcuShared() noexcept = default;

        RcuShared(std::nullptr_t) noexcept
            : mControl(nullptr)
        { }

        template<typename... Args>
        RcuShared(sm::RcuDomain *domain, Args&&... args) noexcept
            : mControl(new (std::nothrow) Counted(domain, std::forward<Args>(args)...))
        { }

        RcuShared(sm::RcuDomain *domain) noexcept
            : mControl(new (std::nothrow) Counted(domain))
        { }

        RcuShared(const RcuShared<T>& other) noexcept
            : mControl(other.mControl)
        {
            if (mControl) {
                mControl->retainStrong(1);
            }
        }

        RcuShared(RcuShared<T>&& other) noexcept
            : mControl(std::exchange(other.mControl, nullptr))
        { }

        RcuShared& operator=(const RcuShared<T>& other) noexcept {
            if (this != &other) {
                Counted *old = std::exchange(mControl, other.mControl);
                if (mControl) {
                    mControl->retainStrong(1);
                }

                if (old) {
                    old->deferReleaseStrong(1);
                }
            }
            return *this;
        }

        RcuShared& operator=(RcuShared<T>&& other) noexcept {
            if (this != &other) {
                Counted *otherControl = std::exchange(other.mControl, nullptr);
                Counted *old = std::exchange(mControl, otherControl);

                if (old) {
                    old->deferReleaseStrong(1);
                }
            }
            return *this;
        }

        ~RcuShared() noexcept {
            reset();
        }

        void reset() noexcept {
            if (Counted *old = std::exchange(mControl, nullptr)) {
                old->deferReleaseStrong(1);
            }
        }

        T *get() const noexcept {
            return mControl ? &mControl->get() : nullptr;
        }

        operator bool() const noexcept {
            return get() != nullptr;
        }
    };
}
