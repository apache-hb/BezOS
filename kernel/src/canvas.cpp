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

km::Canvas::Canvas(boot::FrameBuffer framebuffer, uint8_t *address)
    : mAddress(address)
    , mWidth(framebuffer.width)
    , mHeight(framebuffer.height)
    , mPitch(framebuffer.pitch)
    , mFormat(framebuffer.bpp, framebuffer.redMaskSize, framebuffer.redMaskShift, framebuffer.greenMaskSize, framebuffer.greenMaskShift, framebuffer.blueMaskSize, framebuffer.blueMaskShift)
{ }

km::Canvas::Canvas(Canvas geometry, uint8_t *address)
    : mAddress(address)
    , mWidth(geometry.width())
    , mHeight(geometry.height())
    , mPitch(geometry.pitch())
    , mFormat(geometry.mFormat)
{ }

km::Canvas::PixelValue km::Canvas::pixelValue(Pixel it) const {
    return mFormat.pixelValue(it);
}

uint64_t km::Canvas::pixelOffset(uint64_t x, uint64_t y) const {
    return (y * width() + x) * bytesPerPixel();
}

void km::Canvas::write(uint64_t x, uint64_t y, Pixel pixel) {
    if (x >= mWidth || y >= mHeight)
        return;

    uint64_t offset = pixelOffset(x, y);

    PixelValue pix = pixelValue(pixel);

    memcpy(mAddress + offset, &pix, bytesPerPixel());
}

km::Pixel km::Canvas::read(uint64_t x, uint64_t y) const {
    uint64_t offset = pixelOffset(x, y);
    PixelValue pix = 0;

    memcpy(&pix, mAddress + offset, bytesPerPixel());

    return mFormat.pixelRead(pix);
}

void km::Canvas::fill(Pixel pixel) {
    for (uint64_t x = 0; x < mWidth; x++) {
        for (uint64_t y = 0; y < mHeight; y++) {
            write(x, y, pixel);
        }
    }
}

void km::DrawCharacter(km::Canvas& display, uint64_t x, uint64_t y, char c, Pixel fg, Pixel bg) {
    uint64_t wx = x * 8;
    uint64_t wy = y * 8;
    uint64_t letter = kFontData[(uint8_t)c];

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            Pixel pixel = (letter & (1ull << (i * 8 + j))) ? fg : bg;
            display.write(wx + (8 - j), wy + i, pixel);
        }
    }
}
