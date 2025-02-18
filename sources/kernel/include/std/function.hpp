#pragma once

#include <functional>
#include <utility>

namespace stdx {
    template<typename Signature>
    class Function;

    template<typename R, typename... A>
    class Function<R(A...)> {
        void *mObject;
        R(*mFunction)(void*, A&&...);
        void(*mDelete)(void*);

    public:
        constexpr Function()
            : mObject(nullptr)
            , mFunction(nullptr)
            , mDelete(nullptr)
        { }

        constexpr ~Function() {
            if (mDelete != nullptr) {
                mDelete(mObject);
            }
        }

        template<typename F>
        constexpr Function(F&& func)
            : mObject((void*)(new F(std::forward<F>(func))))
            , mFunction([](void *fn, A&&... args) -> R {
                return std::invoke(*static_cast<F*>(fn), std::forward<A>(args)...);
            })
            , mDelete([](void *fn) { delete static_cast<F*>(fn); })
        { }

        Function(const Function&) = delete;
        Function& operator=(const Function&) = delete;

        constexpr Function(Function&& other)
            : mObject(std::exchange(other.mObject, nullptr))
            , mFunction(std::exchange(other.mFunction, nullptr))
            , mDelete(std::exchange(other.mDelete, nullptr))
        { }

        constexpr Function& operator=(Function&& other) {
            std::swap(*this, other);
        }

        constexpr R invoke(A&&... args) {
            return mFunction(mObject, std::forward<A>(args)...);
        }

        constexpr R operator()(A&&... args) {
            return invoke(std::forward<A>(args)...);
        }

        constexpr operator bool() const {
            return mFunction != nullptr;
        }

        friend void swap(Function& lhs, Function& rhs) {
            std::swap(lhs.mObject, rhs.mObject);
            std::swap(lhs.mFunction, rhs.mFunction);
            std::swap(lhs.mDelete, rhs.mDelete);
        }
    };
}
