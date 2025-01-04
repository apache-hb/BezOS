#pragma once

#include "kernel.hpp"
#include "math.hpp"
#include "memory.hpp"

#include "std/string_view.hpp"

#include "util/util.hpp"

namespace km {
    namespace detail {
        struct Channel {
            uint8_t size;
            uint8_t shift;

            uint16_t maxValue() const;
            uint64_t asValue(uint8_t value) const;
            uint8_t asChannel(uint64_t value) const;
        };
    }

    struct DirtyArea {
        sm::math::int2 min;
        sm::math::int2 max;
    };

    struct Pixel {
        uint8_t r;
        uint8_t g;
        uint8_t b;

        constexpr auto operator<=>(const Pixel&) const = default;
    };

    class PixelFormat {
        uint8_t mBpp;

        detail::Channel mRedChannel;
        detail::Channel mGreenChannel;
        detail::Channel mBlueChannel;

    public:
        constexpr PixelFormat(sm::noinit)
            : mBpp(0)
            , mRedChannel(0, 0)
            , mGreenChannel(0, 0)
            , mBlueChannel(0, 0)
        { }

        PixelFormat(uint8_t bpp, uint8_t redMaskSize, uint8_t redMaskShift, uint8_t greenMaskSize, uint8_t greenMaskShift, uint8_t blueMaskSize, uint8_t blueMaskShift)
            : mBpp(bpp)
            , mRedChannel(redMaskSize, redMaskShift)
            , mGreenChannel(greenMaskSize, greenMaskShift)
            , mBlueChannel(blueMaskSize, blueMaskShift)
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

    class Canvas {
        uint8_t *mAddress;

        uint64_t mWidth;
        uint64_t mHeight;
        uint64_t mPitch;

        PixelFormat mFormat;

        uint64_t pixelValue(Pixel it) const;
        uint64_t pixelOffset(uint64_t x, uint64_t y) const;

    public:
        Canvas(KernelFrameBuffer framebuffer, uint8_t *address);

        Canvas(Canvas geometry, uint8_t *address);

        constexpr Canvas(sm::noinit)
            : mAddress(nullptr)
            , mWidth(0)
            , mHeight(0)
            , mPitch(0)
            , mFormat(sm::noinit{})
        { }

        void write(uint64_t x, uint64_t y, Pixel pixel);

        void fill(Pixel pixel);

        void *address() const { return mAddress; }
        size_t size() const { return mPitch * mHeight; }

        uint64_t width() const { return mWidth; }
        uint64_t height() const { return mHeight; }
        uint64_t pitch() const { return mPitch; }
        uint16_t bpp() const { return mFormat.bpp(); }

        bool hasLinePadding() const { return mWidth != (mPitch / (bpp() / 8)); }

        Pixel read(uint64_t x, uint64_t y) const;
    };

    void DrawCharacter(Canvas& display, uint64_t x, uint64_t y, char c, Pixel fg, Pixel bg);

    class Terminal {
        Canvas mDisplay;

        uint16_t mCurrentRow;
        uint16_t mCurrentColumn;

        uint16_t mRowCount;
        uint16_t mColumnCount;

        void put(char c);

        void advance();
        void newline();

        void write(uint64_t x, uint64_t y, char c);

    public:
        Terminal(Canvas display)
            : mDisplay(display)
            , mCurrentRow(0)
            , mCurrentColumn(0)
            , mRowCount(display.height() / 8)
            , mColumnCount(display.width() / 8)
        { }

        constexpr Terminal(sm::noinit)
            : mDisplay(sm::noinit{})
            , mCurrentRow(0)
            , mCurrentColumn(0)
            , mRowCount(0)
            , mColumnCount(0)
        { }

        void print(stdx::StringView message);

        Canvas& display() { return mDisplay; }
    };

    class BufferedTerminal {
        Canvas mDisplay;
        Canvas mBackBuffer;

        uint16_t mCurrentRow;
        uint16_t mCurrentColumn;

        uint16_t mRowCount;
        uint16_t mColumnCount;

        void put(char c);

        void advance();
        void newline();

        void write(uint64_t x, uint64_t y, char c);

    public:
        BufferedTerminal(Canvas display, SystemMemory& memory);

        constexpr BufferedTerminal(sm::noinit)
            : mDisplay(sm::noinit{})
            , mBackBuffer(sm::noinit{})
            , mCurrentRow(0)
            , mCurrentColumn(0)
            , mRowCount(0)
            , mColumnCount(0)
        { }

        void print(stdx::StringView message);
    };
}
