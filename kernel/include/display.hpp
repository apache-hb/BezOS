#pragma once

#include "kernel.hpp"
#include "std/string_view.hpp"

#include "util/util.hpp"

namespace km {
    struct Pixel {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };

    class Display {
        uint8_t *mAddress;

        uint64_t mWidth;
        uint64_t mHeight;
        uint64_t mPitch;
        uint16_t mBpp;

        uint8_t mRedMaskSize;
        uint8_t mRedMaskShift;

        uint8_t mGreenMaskSize;
        uint8_t mGreenMaskShift;

        uint8_t mBlueMaskSize;
        uint8_t mBlueMaskShift;

        uint32_t pixelValue(Pixel it) const;

    public:
        Display(KernelFrameBuffer framebuffer, uint8_t *address);

        constexpr Display(sm::noinit)
            : mAddress(nullptr)
            , mWidth(0)
            , mHeight(0)
            , mPitch(0)
            , mBpp(0)
            , mRedMaskSize(0)
            , mRedMaskShift(0)
            , mGreenMaskSize(0)
            , mGreenMaskShift(0)
            , mBlueMaskSize(0)
            , mBlueMaskShift(0)
        { }

        void write(uint64_t x, uint64_t y, Pixel pixel);

        void fill(Pixel pixel);

        void *address() const { return mAddress; }
        size_t size() const { return mWidth * mPitch * mBpp / 8; }

        uint64_t width() const { return mWidth; }
        uint64_t height() const { return mHeight; }
        uint64_t pitch() const { return mPitch; }
        uint16_t bpp() const { return mBpp; }
    };

    class Terminal {
        static constexpr size_t kColumnCount = 80;
        static constexpr size_t kRowCount = 25;

        Display mDisplay;

        uint16_t mCurrentRow;
        uint16_t mCurrentColumn;

        char buffer[kColumnCount * kRowCount] = { };

        void put(char c);

        void advance();
        void newline();

        void write(uint64_t x, uint64_t y, char c);

    public:
        Terminal(Display display)
            : mDisplay(display)
            , mCurrentRow(0)
            , mCurrentColumn(0)
        {
            std::fill(std::begin(buffer), std::end(buffer), ' ');
        }

        constexpr Terminal(sm::noinit)
            : mDisplay(sm::noinit{})
            , mCurrentRow(0)
            , mCurrentColumn(0)
        {
            std::fill(std::begin(buffer), std::end(buffer), ' ');
        }

        void print(stdx::StringView message);

        Display& display() { return mDisplay; }
    };
}
