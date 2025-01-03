#pragma once

#include "kernel.hpp"
#include "std/string_view.hpp"

#include "util/util.hpp"

namespace km {
    struct Pixel {
        uint8_t r;
        uint8_t g;
        uint8_t b;

        constexpr auto operator<=>(const Pixel&) const = default;
    };

    class PixelFormat {
        uint8_t mBpp;

        uint8_t mRedMaskSize;
        uint8_t mRedMaskShift;

        uint8_t mGreenMaskSize;
        uint8_t mGreenMaskShift;

        uint8_t mBlueMaskSize;
        uint8_t mBlueMaskShift;

    public:
        PixelFormat(uint8_t bpp, uint8_t redMaskSize, uint8_t redMaskShift, uint8_t greenMaskSize, uint8_t greenMaskShift, uint8_t blueMaskSize, uint8_t blueMaskShift)
            : mBpp(bpp)
            , mRedMaskSize(redMaskSize)
            , mRedMaskShift(redMaskShift)
            , mGreenMaskSize(greenMaskSize)
            , mGreenMaskShift(greenMaskShift)
            , mBlueMaskSize(blueMaskSize)
            , mBlueMaskShift(blueMaskShift)
        { }

        /// @brief Stores a pixel value as it would be stored in the framebuffer exactly.
        using PixelValue = uint64_t;

        /// @brief Stores the value of a single colour channel in a pixel.
        /// This is always a value between 0 and 255, while we support displays
        /// with a higher bit depth, i only care about displaying 8-bit colour.
        /// This is a kernel bootlog console, not a graphics engine.
        using ChannelValue = uint8_t;

        uint8_t bpp() const { return mBpp; }

        PixelValue pixelValue(Pixel it) const;

        ChannelValue getRedChannel(PixelValue value) const;
        ChannelValue getGreenChannel(PixelValue value) const;
        ChannelValue getBlueChannel(PixelValue value) const;

        Pixel pixelRead(PixelValue value) const;
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
        uint64_t pixelOffset(uint64_t x, uint64_t y) const;

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
        size_t size() const { return mPitch * mHeight * mBpp / 8; }

        uint64_t width() const { return mWidth; }
        uint64_t height() const { return mHeight; }
        uint64_t pitch() const { return mPitch; }
        uint16_t bpp() const { return mBpp; }
        uint8_t pixelSize() const { return mBpp / 8; }

        bool hasLinePadding() const { return mWidth != (mPitch / pixelSize()); }

        Pixel read(uint64_t x, uint64_t y) const;
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
