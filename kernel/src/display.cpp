#include "display.hpp"

#include <string.h>

#include "font.hpp"

#include "kernel.hpp"

km::Display::Display(KernelFrameBuffer framebuffer, uint8_t *address)
    : mAddress(address)
    , mWidth(framebuffer.width)
    , mHeight(framebuffer.height)
    , mPitch(framebuffer.pitch)
    , mBpp(framebuffer.bpp)
    , mRedMaskSize(framebuffer.redMaskSize)
    , mRedMaskShift(framebuffer.redMaskShift)
    , mGreenMaskSize(framebuffer.greenMaskSize)
    , mGreenMaskShift(framebuffer.greenMaskShift)
    , mBlueMaskSize(framebuffer.blueMaskSize)
    , mBlueMaskShift(framebuffer.blueMaskShift)
{ }

uint32_t km::Display::pixelValue(Pixel it) const {
    return uint32_t(it.r & (1 << mRedMaskSize) - 1) << mRedMaskShift
         | uint32_t(it.g & (1 << mGreenMaskSize) - 1) << mGreenMaskShift
         | uint32_t(it.b & (1 << mBlueMaskSize) - 1) << mBlueMaskShift;
}

uint64_t km::Display::pixelOffset(uint64_t x, uint64_t y) const {
    return (y * (mPitch / 4) + x) * (mBpp / 8);
}

void km::Display::write(uint64_t x, uint64_t y, Pixel pixel) {
    if (x >= mWidth || y >= mHeight)
        return;

    uint64_t offset = pixelOffset(x, y);

    uint32_t pix = pixelValue(pixel);

    memcpy(mAddress + offset, &pix, mBpp / 8);
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
