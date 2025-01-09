#include <gtest/gtest.h>

#include <cstdlib>

#include "display.hpp"

TEST(CanvasTest, Size) {
    KernelFrameBuffer framebuffer = {
        .width = 1280,
        .height = 800,
        .pitch = 5120,
        .bpp = 32,
        .redMaskSize = 8,
        .redMaskShift = 16,
        .greenMaskSize = 8,
        .greenMaskShift = 8,
        .blueMaskSize = 8,
        .blueMaskShift = 0,
    };

    std::unique_ptr<uint8_t[]> address(new uint8_t[framebuffer.size()]);

    km::Canvas display(framebuffer, address.get());

    ASSERT_EQ(display.size(), (1280 * 800 * 4));

    ASSERT_EQ(display.rowSize(), (1280 * 4));
}

TEST(DisplayTest, Simple) {
    KernelFrameBuffer framebuffer = {
        .width = 1280,
        .height = 800,
        .pitch = 5120,
        .bpp = 32,
        .redMaskSize = 8,
        .redMaskShift = 16,
        .greenMaskSize = 8,
        .greenMaskShift = 8,
        .blueMaskSize = 8,
        .blueMaskShift = 0,
    };

    std::unique_ptr<uint8_t[]> address(new uint8_t[framebuffer.size()]);

    km::Canvas display(framebuffer, address.get());

    ASSERT_EQ(display.size(), framebuffer.size());
    ASSERT_EQ(display.width(), framebuffer.width);
    ASSERT_EQ(display.height(), framebuffer.height);
    ASSERT_EQ(display.pitch(), framebuffer.pitch);
    ASSERT_EQ(display.bpp(), framebuffer.bpp);

    ASSERT_FALSE(display.hasLinePadding());

    display.write(0, 0, km::Pixel { 255, 255, 255 });

    km::Pixel pixel = display.read(0, 0);

    ASSERT_EQ(pixel.r, 255);
    ASSERT_EQ(pixel.g, 255);
    ASSERT_EQ(pixel.b, 255);
}

TEST(DisplayTest, WithLinePadding) {
    KernelFrameBuffer framebuffer = {
        .width = 1920,
        .height = 1080,
        .pitch = 1920 * 4 + 4,
        .bpp = 32,
        .redMaskSize = 8,
        .redMaskShift = 16,
        .greenMaskSize = 8,
        .greenMaskShift = 8,
        .blueMaskSize = 8,
        .blueMaskShift = 0,
    };

    std::unique_ptr<uint8_t[]> address(new uint8_t[framebuffer.size()]);
    memset(address.get(), 0, framebuffer.size());

    km::Canvas display(framebuffer, address.get());

    ASSERT_EQ(display.size(), framebuffer.size());
    ASSERT_EQ(display.width(), framebuffer.width);
    ASSERT_EQ(display.height(), framebuffer.height);
    ASSERT_EQ(display.pitch(), framebuffer.pitch);
    ASSERT_EQ(display.bpp(), framebuffer.bpp);

    ASSERT_TRUE(display.hasLinePadding());

    display.write(0, 0, km::Pixel { 255, 255, 255 });

    km::Pixel pixel = display.read(0, 0);

    ASSERT_EQ(pixel.r, 255);
    ASSERT_EQ(pixel.g, 255);
    ASSERT_EQ(pixel.b, 255);
}

struct PixelFormatValueTest
    : public testing::TestWithParam<km::PixelFormat> {};

INSTANTIATE_TEST_SUITE_P(
    LowColourDepth, PixelFormatValueTest,
    testing::Values(
        /// High colour formats use 5 or 6 bits of depth per pixel
        /// https://en.wikipedia.org/wiki/Color_depth#High_color_(15/16-bit)

        // 5 bit formats
        km::PixelFormat(32, 5, 11, 5, 6, 5, 0),
        km::PixelFormat(32, 5, 0, 5, 6, 5, 11),
        km::PixelFormat(32, 5, 6, 5, 6, 5, 0),

        // 5 bit formats with 24 bpp
        km::PixelFormat(24, 5, 11, 5, 6, 5, 0),
        km::PixelFormat(24, 5, 0, 5, 6, 5, 11),
        km::PixelFormat(24, 5, 6, 5, 6, 5, 0),

        // 5 bit formats with 15 bpp
        km::PixelFormat(15, 5, 11, 5, 6, 5, 0),
        km::PixelFormat(15, 5, 0, 5, 6, 5, 11),
        km::PixelFormat(15, 5, 6, 5, 6, 5, 0),

        // 6 bit formats
        km::PixelFormat(32, 6, 10, 6, 10, 6, 0),
        km::PixelFormat(32, 6, 0, 6, 10, 6, 10),
        km::PixelFormat(32, 6, 10, 6, 10, 6, 10),

        // 6 bit formats with 24 bpp
        km::PixelFormat(24, 6, 10, 6, 10, 6, 0),
        km::PixelFormat(24, 6, 0, 6, 10, 6, 10),
        km::PixelFormat(24, 6, 10, 6, 10, 6, 10),

        // 6 bit formats with 18 bpp
        km::PixelFormat(18, 6, 10, 6, 10, 6, 0),
        km::PixelFormat(18, 6, 0, 6, 10, 6, 10),
        km::PixelFormat(18, 6, 10, 6, 10, 6, 10),
    ));

INSTANTIATE_TEST_SUITE_P(
    PixelValue, PixelFormatValueTest,
    testing::Values(
        /// True colour format uses 8 bits of depth per pixel
        /// https://en.wikipedia.org/wiki/Color_depth#True_color_(24-bit)

        // True colour (32-bit)
        km::PixelFormat(32, 8, 0, 8, 8, 8, 16),
        km::PixelFormat(32, 8, 16, 8, 8, 8, 0),
        km::PixelFormat(32, 8, 8, 8, 8, 8, 8),

        // True colour (24-bit)
        km::PixelFormat(24, 8, 0, 8, 8, 8, 16),
        km::PixelFormat(24, 8, 16, 8, 8, 8, 0),
        km::PixelFormat(24, 8, 8, 8, 8, 8, 8),

        // True colour (64-bit)
        km::PixelFormat(64, 8, 0, 8, 8, 8, 16),
        km::PixelFormat(64, 8, 16, 8, 8, 8, 0),
        km::PixelFormat(64, 8, 8, 8, 8, 8, 8),
    ));

TEST_P(PixelFormatValueTest, PixelValue) {
    auto format = GetParam();

    km::Pixel pixel = { 255, 255, 255 };

    uint32_t value = format.pixelValue(pixel);

    km::Pixel read = format.pixelRead(value);

    ASSERT_EQ(read, pixel);
}

TEST_P(PixelFormatValueTest, LowColourDepth) {
    auto format = GetParam();

    km::Pixel pixel = { 255, 255, 255 };

    uint32_t value = format.pixelValue(pixel);

    km::Pixel read = format.pixelRead(value);

    ASSERT_EQ(read, pixel);
}

#if 0
struct DisplayPixelTest
    : public testing::TestWithParam<KernelFrameBuffer> {};

INSTANTIATE_TEST_SUITE_P(
    OddPixelShape, DisplayPixelTest,
    testing::Values(
        KernelFrameBuffer {
            .width = 1920,
            .height = 1080,
            .pitch = 1920 * 4 + 4,
            .bpp = 32,
            .redMaskSize = 8,
            .redMaskShift = 16,
            .greenMaskSize = 8,
            .greenMaskShift = 8,
            .blueMaskSize = 8,
            .blueMaskShift = 0,
        },
        KernelFrameBuffer {
            .width = 1280,
            .height = 800,
            .pitch = 5120,
            .bpp = 32,
            .redMaskSize = 8,
            .redMaskShift = 16,
            .greenMaskSize = 8,
            .greenMaskShift = 8,
            .blueMaskSize = 8,
            .blueMaskShift = 0,
        },
        // non 32-bit bpp
        KernelFrameBuffer {
            .width = 1920,
            .height = 1080,
            .pitch = 1920 * 3,
            .bpp = 24,
            .redMaskSize = 8,
            .redMaskShift = 16,
            .greenMaskSize = 8,
            .greenMaskShift = 8,
            .blueMaskSize = 8,
            .blueMaskShift = 0,
        },
        // weirdly large bpp
        KernelFrameBuffer {
            .width = 1920,
            .height = 1080,
            .pitch = 1920 * 8,
            .bpp = 64,
            .redMaskSize = 16,
            .redMaskShift = 32,
            .greenMaskSize = 16,
            .greenMaskShift = 16,
            .blueMaskSize = 16,
            .blueMaskShift = 0,
        },
        // reordered masks
        KernelFrameBuffer {
            .width = 1920,
            .height = 1080,
            .pitch = 1920 * 4,
            .bpp = 32,
            .redMaskSize = 8,
            .redMaskShift = 0,
            .greenMaskSize = 8,
            .greenMaskShift = 16,
            .blueMaskSize = 8,
            .blueMaskShift = 8,
        },
        // non power2 masks
        KernelFrameBuffer {
            .width = 1920,
            .height = 1080,
            .pitch = 1920 * 4,
            .bpp = 32,
            .redMaskSize = 5,
            .redMaskShift = 0,
            .greenMaskSize = 6,
            .greenMaskShift = 5,
            .blueMaskSize = 5,
            .blueMaskShift = 11,
        }
    ));

TEST_P(DisplayPixelTest, OddPixelShape) {
    auto framebuffer = GetParam();

    void *address = std::malloc(framebuffer.size());

    km::Display display(framebuffer, (uint8_t*)address);

    ASSERT_EQ(display.size(), framebuffer.size());
    ASSERT_EQ(display.width(), framebuffer.width);
    ASSERT_EQ(display.height(), framebuffer.height);
    ASSERT_EQ(display.pitch(), framebuffer.pitch);
    ASSERT_EQ(display.bpp(), framebuffer.bpp);

    // read and write to each corner

    // top left
    km::Pixel pixel = { 255, 255, 255 };
    display.write(0, 0, pixel);
    km::Pixel read = display.read(0, 0);
    ASSERT_EQ(read, pixel);

    // top right
    display.write(framebuffer.width - 1, 0, pixel);
    read = display.read(framebuffer.width - 1, 0);
    ASSERT_EQ(read, pixel);

    // bottom left
    display.write(0, framebuffer.height - 1, pixel);
    read = display.read(0, framebuffer.height - 1);
    ASSERT_EQ(read, pixel);

    // bottom right
    display.write(framebuffer.width - 1, framebuffer.height - 1, pixel);
    read = display.read(framebuffer.width - 1, framebuffer.height - 1);
    ASSERT_EQ(read, pixel);
}
#endif
