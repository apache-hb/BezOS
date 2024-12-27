#include "display.hpp"

#include <string.h>

#include "font.hpp"

#include "kernel.hpp"

km::Display::Display(limine_framebuffer framebuffer)
    : mAddress((uint8_t*)framebuffer.address)
    , mWidth(framebuffer.width)
    , mHeight(framebuffer.height)
    , mPitch(framebuffer.pitch)
    , mBpp(framebuffer.bpp)
    , mRedMaskSize(framebuffer.red_mask_size)
    , mRedMaskShift(framebuffer.red_mask_shift)
    , mGreenMaskSize(framebuffer.green_mask_size)
    , mGreenMaskShift(framebuffer.green_mask_shift)
    , mBlueMaskSize(framebuffer.blue_mask_size)
    , mBlueMaskShift(framebuffer.blue_mask_shift)
{
    KmDebugMessage("[INIT] Display: ", mWidth, "x", mHeight, "x", mBpp, " @ ", Hex((uintptr_t)mAddress).pad(16, '0'), "\n");
    KmDebugMessage("[INIT] Display: Pitch: ", mPitch / 4, "\n");
    KmDebugMessage("[INIT] Display: Red[", mRedMaskSize, ":", mRedMaskShift, "], Green[", mGreenMaskSize, ":", mGreenMaskShift, "], Blue[", mBlueMaskSize, ":", mBlueMaskShift, "]\n");
}

uint32_t km::Display::pixelValue(Pixel it) const {
    return uint32_t(it.r & (1 << mRedMaskSize) - 1) << mRedMaskShift
         | uint32_t(it.g & (1 << mGreenMaskSize) - 1) << mGreenMaskShift
         | uint32_t(it.b & (1 << mBlueMaskSize) - 1) << mBlueMaskShift;
}

void km::Display::write(uint64_t x, uint64_t y, Pixel pixel) {
    if (x >= mWidth || y >= mHeight)
        return;

    uint64_t offset = (y * (mPitch / 4) + x) * (mBpp / 8);

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
    uint64_t x = mCurrentColumn * 8;
    uint64_t y = mCurrentRow * 8;
    uint64_t letter = kFontData[(uint8_t)c];

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            Pixel pixel = (letter & (1ull << (i * 8 + j))) ? Pixel { 255, 255, 255 } : Pixel { 0, 0, 0 };
            mDisplay.write(x + (8 - j), y + i, pixel);
        }
    }

    advance();
}

void km::Terminal::advance() {
    mCurrentColumn += 1;

    if (mCurrentColumn >= mColumnCount) {
        newline();
    }
}

void km::Terminal::newline() {
    mCurrentRow += 1;
    if (mCurrentRow >= mRowCount) {
        mCurrentRow = 0;
    }

    mCurrentColumn = 0;

    // clear current row
    for (uint64_t x = 0; x < mDisplay.width(); x++) {
        for (uint64_t y = mCurrentRow * 8; y < (mCurrentRow + 1) * 8; y++) {
            mDisplay.write(x, y, Pixel { 0, 0, 0 });
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
