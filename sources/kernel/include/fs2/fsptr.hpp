#pragma once

#include <concepts>

namespace vfs2 {
    template<typename T>
    concept RefCounted = requires (T it) {
        { it.retain() } -> std::same_as<void>;
        { it.release() } -> std::same_as<unsigned>;
    };

    template<typename T>
    class AutoRelease {
        T *mObject = nullptr;

        void retain() {
            if (mObject) {
                mObject->retain();
            }
        }

        void release() {
            if (mObject) {
                mObject->release();
            }

            mObject = nullptr;
        }

    public:
        using type = T;

        constexpr AutoRelease()
            : AutoRelease(nullptr)
        { }

        constexpr AutoRelease(T *object)
            : mObject(object)
        { }

        constexpr ~AutoRelease() {
            release();
        }

        explicit constexpr AutoRelease(const AutoRelease& other)
            : mObject(other.mObject)
        {
            retain();
        }

        constexpr AutoRelease(AutoRelease&& other) {
            std::swap(mObject, other.mObject);
        }

        constexpr AutoRelease& operator=(const AutoRelease& other) {
            if (this != &other) {
                release();
                mObject = other.mObject;
                retain();
            }

            return *this;
        }

        constexpr AutoRelease& operator=(AutoRelease&& other) {
            if (this != &other) {
                std::swap(mObject, other.mObject);
            }

            return *this;
        }

        constexpr auto detach() {
            T *object = mObject;
            mObject = nullptr;
            return object;
        }

        constexpr auto get(this auto&& self) {
            return self.mObject;
        }

        constexpr auto addressOf(this auto&& self) {
            return &self.mObject;
        }

        constexpr auto operator->(this auto&& self) {
            return self.mObject;
        }

        constexpr auto operator*() {
            return *mObject;
        }

        constexpr operator bool() const {
            return mObject != nullptr;
        }

        constexpr void reset(T *object) {
            release();
            mObject = object;
        }

        friend void swap(AutoRelease& a, AutoRelease& b) {
            std::swap(a.mObject, b.mObject);
        }
    };
}
