#pragma once

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

        uint8_t redMaskSize() const { return mRedChannel.size; }
        uint8_t redMaskShift() const { return mRedChannel.shift; }
        uint8_t greenMaskSize() const { return mGreenChannel.size; }
        uint8_t greenMaskShift() const { return mGreenChannel.shift; }
        uint8_t blueMaskSize() const { return mBlueChannel.size; }
        uint8_t blueMaskShift() const { return mBlueChannel.shift; }

        Pixel pixelRead(PixelValue value) const;
    };

    class Canvas {
        km::PhysicalAddress mPhysical;
        uint8_t *mAddress;

        uint64_t mWidth;
        uint64_t mHeight;
        uint64_t mPitch;

        PixelFormat mFormat;

        using PixelValue = PixelFormat::PixelValue;

        PixelValue pixelValue(Pixel it) const;
        uint64_t pixelOffset(uint64_t x, uint64_t y) const;

    public:
        Canvas(boot::FrameBuffer framebuffer, uint8_t *address);

        Canvas(Canvas geometry, uint8_t *address);

        constexpr Canvas(sm::noinit)
            : mPhysical(nullptr)
            , mAddress(nullptr)
            , mWidth(0)
            , mHeight(0)
            , mPitch(0)
            , mFormat(sm::noinit{})
        { }

        void write(uint64_t x, uint64_t y, Pixel pixel);

        void fill(Pixel pixel);

        km::PhysicalAddress physical() const { return mPhysical; }
        void *address() const { return mAddress; }
        void setAddress(void *address) { mAddress = (uint8_t*)address; }
        size_t size() const { return mPitch * mHeight; }

        uint64_t width() const { return mWidth; }
        uint64_t height() const { return mHeight; }
        uint64_t pitch() const { return mPitch; }
        uint16_t bpp() const { return mFormat.bpp(); }
        uint8_t bytesPerPixel() const { return bpp() / CHAR_BIT; }

        size_t offset(uint64_t x, uint64_t y) const { return pixelOffset(x, y); }
        size_t stride() const { return width() * bytesPerPixel(); }

        bool hasLinePadding() const { return mWidth != (pitch() / bytesPerPixel()); }

        uint8_t redMaskSize() const { return mFormat.redMaskSize(); }
        uint8_t redMaskShift() const { return mFormat.redMaskShift(); }
        uint8_t greenMaskSize() const { return mFormat.greenMaskSize(); }
        uint8_t greenMaskShift() const { return mFormat.greenMaskShift(); }
        uint8_t blueMaskSize() const { return mFormat.blueMaskSize(); }
        uint8_t blueMaskShift() const { return mFormat.blueMaskShift(); }

        Pixel read(uint64_t x, uint64_t y) const;
    };

    void TestCanvas(Canvas& canvas);

    void DrawCharacter(Canvas& display, uint64_t x, uint64_t y, char c, Pixel fg, Pixel bg);

    class DirectTerminal final {
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
        DirectTerminal(Canvas display)
            : mDisplay(display)
            , mCurrentRow(0)
            , mCurrentColumn(0)
            , mRowCount(display.height() / 8)
            , mColumnCount(display.width() / 8)
        { }

        constexpr DirectTerminal(sm::noinit)
            : mDisplay(sm::noinit{})
            , mCurrentRow(0)
            , mCurrentColumn(0)
            , mRowCount(0)
            , mColumnCount(0)
        { }

        void print(stdx::StringView message);

        Canvas& display() { return mDisplay; }
        uint16_t currentRow() const { return mCurrentRow; }
        uint16_t currentColumn() const { return mCurrentColumn; }
        uint16_t rowCount() const { return mRowCount; }
        uint16_t columnCount() const { return mColumnCount; }

        void clear();
    };

    class BufferedTerminal final {
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

        /// @brief Allocates a canvas in system memory.
        ///
        /// @param geometry The geometry of the canvas.
        /// @param memory The system memory allocator.
        ///
        /// @return The allocated canvas.
        static km::Canvas InMemoryCanvas(km::Canvas geometry, km::SystemMemory& memory) {
            size_t size = geometry.size();
            uint8_t *data = (uint8_t*)memory.allocate(size);
            memset(data, 0, size);
            return km::Canvas(geometry, data);
        }

    public:
        BufferedTerminal(Canvas display, SystemMemory& memory)
            : BufferedTerminal(display, InMemoryCanvas(display, memory))
        { }

        BufferedTerminal(DirectTerminal terminal, SystemMemory& memory)
            : BufferedTerminal(terminal.display(), InMemoryCanvas(terminal.display(), memory))
        { }

        BufferedTerminal(Canvas display, Canvas backBuffer);

        constexpr BufferedTerminal(sm::noinit)
            : mDisplay(sm::noinit{})
            , mBackBuffer(sm::noinit{})
            , mCurrentRow(0)
            , mCurrentColumn(0)
            , mRowCount(0)
            , mColumnCount(0)
        { }

        void print(stdx::StringView message);

        void clear();
    };
}
