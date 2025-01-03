#include "display.hpp"

#include <string.h>

#include "font.hpp"

#include "kernel.hpp"

uint16_t km::detail::Channel::maxValue() const { return (1 << size) - 1; }

uint64_t km::detail::Channel::asValue(uint8_t value) const {
    uint16_t max = maxValue();
    uint16_t c = value * max / 255;
    return uint64_t(c & max) << shift;
}

uint8_t km::detail::Channel::asChannel(uint64_t value) const {
    uint16_t max = maxValue();
    uint16_t c = (value >> shift) & max;
    return c * 255 / max;
}

km::PixelFormat::PixelValue km::PixelFormat::pixelValue(km::Pixel it) const {
    return mRedChannel.asValue(it.r)
         | mGreenChannel.asValue(it.g)
         | mBlueChannel.asValue(it.b);
}

km::PixelFormat::ChannelValue km::PixelFormat::getRedChannel(PixelValue value) const {
    return mRedChannel.asChannel(value);
}

km::PixelFormat::ChannelValue km::PixelFormat::getGreenChannel(PixelValue value) const {
    return mGreenChannel.asChannel(value);
}

km::PixelFormat::ChannelValue km::PixelFormat::getBlueChannel(PixelValue value) const {
    return mBlueChannel.asChannel(value);
}

km::Pixel km::PixelFormat::pixelRead(PixelValue value) const {
    ChannelValue r = getRedChannel(value);
    ChannelValue g = getGreenChannel(value);
    ChannelValue b = getBlueChannel(value);

    return Pixel { uint8_t(r), uint8_t(g), uint8_t(b) };
}

km::Display::Display(KernelFrameBuffer framebuffer, uint8_t *address)
    : mAddress(address)
    , mWidth(framebuffer.width)
    , mHeight(framebuffer.height)
    , mPitch(framebuffer.pitch)
    , mFormat(framebuffer.bpp, framebuffer.redMaskSize, framebuffer.redMaskShift, framebuffer.greenMaskSize, framebuffer.greenMaskShift, framebuffer.blueMaskSize, framebuffer.blueMaskShift)
{ }

uint64_t km::Display::pixelValue(Pixel it) const {
    return mFormat.pixelValue(it);
}

uint64_t km::Display::pixelOffset(uint64_t x, uint64_t y) const {
    return (y * (mPitch / 4) + x) * (bpp() / 8);
}

void km::Display::write(uint64_t x, uint64_t y, Pixel pixel) {
    if (x >= mWidth || y >= mHeight)
        return;

    uint64_t offset = pixelOffset(x, y);

    uint32_t pix = pixelValue(pixel);

    memcpy(mAddress + offset, &pix, bpp() / 8);
}

km::Pixel km::Display::read(uint64_t x, uint64_t y) const {
    uint8_t offset = pixelOffset(x, y);
    uint32_t *pix = (uint32_t*)(mAddress + offset);

    return mFormat.pixelRead(*pix);
}

void km::Display::fill(Pixel pixel) {
    for (uint64_t x = 0; x < mWidth; x++) {
        for (uint64_t y = 0; y < mHeight; y++) {
            write(x, y, pixel);
        }
    }
}

void km::Terminal::put(char c) {
    write(mCurrentColumn, mCurrentRow, c);
    buffer[mCurrentRow * kColumnCount + mCurrentColumn] = c;

    advance();
}

void km::Terminal::write(uint64_t x, uint64_t y, char c) {
    x *= 8;
    y *= 8;
    uint64_t letter = kFontData[(uint8_t)c];

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            Pixel pixel = (letter & (1ull << (i * 8 + j))) ? Pixel { 255, 255, 255 } : Pixel { 0, 0, 0 };
            mDisplay.write(x + (8 - j), y + i, pixel);
        }
    }
}

void km::Terminal::advance() {
    mCurrentColumn += 1;

    if (mCurrentColumn >= kColumnCount) {
        newline();
        mCurrentColumn = 0;
    }
}

void km::Terminal::newline() {
    mCurrentRow += 1;
    bool scroll = false;
    if (mCurrentRow >= kRowCount) {
        mCurrentRow = kRowCount - 1;
        scroll = true;
    }

    mCurrentColumn = 0;

    if (scroll) {
        memmove(buffer, buffer + kColumnCount, kColumnCount * (kRowCount - 1));
        memset(buffer + kColumnCount * (kRowCount - 1), ' ', kColumnCount);

        // redraw the screen
        for (uint64_t y = 0; y < kRowCount; y++) {
            for (uint64_t x = 0; x < kColumnCount; x++) {
                write(x, y, buffer[y * kColumnCount + x]);
            }
        }
    }
}

void km::Terminal::print(stdx::StringView message) {
    for (char c : message) {
        if (c == '\0') {
            continue;
        }

        if (c == '\n') {
            newline();
        } else {
            put(c);
        }
    }
}
