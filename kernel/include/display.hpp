#pragma once

#include "std/static_vector.hpp"
#include "std/string_view.hpp"

#include "util/util.hpp"

#include <limine.h>

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
        Display(limine_framebuffer framebuffer);

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
        size_t size() const { return mWidth * mHeight * mBpp / 8; }

        uint64_t width() const { return mWidth; }
        uint64_t height() const { return mHeight; }
        uint64_t pitch() const { return mPitch; }
        uint16_t bpp() const { return mBpp; }
    };

    class Terminal {
        Display mDisplay;

        uint16_t mCurrentRow;
        uint16_t mCurrentColumn;

        uint16_t mRowCount;
        uint16_t mColumnCount;

        void put(char c);

        void advance();
        void newline();

    public:
        Terminal(Display display, uint16_t rows, uint16_t columns)
            : mDisplay(display)
            , mCurrentRow(0)
            , mCurrentColumn(0)
            , mRowCount(rows)
            , mColumnCount(columns)
        { }

        constexpr Terminal(sm::noinit)
            : mDisplay(sm::noinit{})
            , mCurrentRow(0)
            , mCurrentColumn(0)
            , mRowCount(0)
            , mColumnCount(0)
        { }

        void print(stdx::StringView message);

        Display& display() { return mDisplay; }
    };
}
