#include <gtest/gtest.h>

#include <cstdlib>

#include <stb_image_write.h>

#include "display.hpp"

static constexpr stdx::StringView kText = R"(
| /SYS/PCI/00.1F.00 | Revision             | 2
| /SYS/PCI/00.1F.00 | Type                 | Multi-Function
| /SYS/PCI/00.1F.02 | Vendor ID            | Intel (0x8086)
| /SYS/PCI/00.1F.02 | Device ID            | 0x2922
| /SYS/PCI/00.1F.02 | Class                | DeviceClass(class: 0x01, subclass: 0x06)
| /SYS/PCI/00.1F.02 | Programmable         | 1
| /SYS/PCI/00.1F.02 | Revision             | 2
| /SYS/PCI/00.1F.02 | Type                 | Multi-Function
| /SYS/PCI/00.1F.03 | Vendor ID            | Intel (0x8086)
| /SYS/PCI/00.1F.03 | Device ID            | 0x2930
| /SYS/PCI/00.1F.03 | Class                | DeviceClass(class: 0x0C, subclass: 0x05)
| /SYS/PCI/00.1F.03 | Programmable         | 0
| /SYS/PCI/00.1F.03 | Revision             | 2
| /SYS/PCI/00.1F.03 | Type                 | Multi-Function
[SMP] Starting APs.
[SMP] BSP ID: 0
[SMP] Starting APIC ID: 1
[SMP] Starting Core.
[APIC] Startup: 0xFEE00800, Base address: 0xFEE00000, State: Enabled, Type: Application Processor
[SMP] Started AP 1
[SMP] Starting APIC ID: 2
[SMP] Starting Core.
[APIC] Startup: 0xFEE00800, Base address: 0xFEE00000, State: Enabled, Type: Application Processor
[SMP] Started AP 2
[SMP] Starting APIC ID: 3
[SMP] Starting Core.
[APIC] Startup: 0xFEE00800, Base address: 0xFEE00000, State: Enabled, Type: Application Processor
[SMP] Started AP 3
[SMP] Starting APIC ID: 4
[SMP] Starting Core.
[APIC] Startup: 0xFEE00800, Base address: 0xFEE00000, State: Enabled, Type: Application Processor
[SMP] Started AP 4
[SMP] Starting APIC ID: 5
[SMP] Starting Core.
[APIC] Startup: 0xFEE00800, Base address: 0xFEE00000, State: Enabled, Type: Application Processor
[SMP] Started AP 5
[SMP] Starting APIC ID: 6
[SMP] Starting Core.
[APIC] Startup: 0xFEE00800, Base address: 0xFEE00000, State: Enabled, Type: Application Processor
[SMP] Started AP 6
[SMP] Starting APIC ID: 7
[SMP] Starting Core.
[APIC] Startup: 0xFEE00800, Base address: 0xFEE00000, State: Enabled, Type: Application Processor
[SMP] Started AP 7
[SMP] Starting APIC ID: 8
[SMP] Starting Core.
[APIC] Startup: 0xFEE00800, Base address: 0xFEE00000, State: Enabled, Type: Application Processor
[SMP] Started AP 8
[SMP] Starting APIC ID: 9
[SMP] Starting Core.
[APIC] Startup: 0xFEE00800, Base address: 0xFEE00000, State: Enabled, Type: Application Processor
[SMP] Started AP 9
[SMP] Starting APIC ID: 10
[SMP] Starting Core.
[APIC] Startup: 0xFEE00800, Base address: 0xFEE00000, State: Enabled, Type: Application Processor
[SMP] Started AP 10
[SMP] Starting APIC ID: 11
[SMP] Starting Core.
[APIC] Startup: 0xFEE00800, Base address: 0xFEE00000, State: Enabled, Type: Application Processor
[SMP] Started AP 11
[SMP] Starting APIC ID: 12
[SMP] Starting Core.
[APIC] Startup: 0xFEE00800, Base address: 0xFEE00000, State: Enabled, Type: Application Processor
[SMP] Started AP 12
[SMP] Starting APIC ID: 13
[SMP] Starting Core.
[APIC] Startup: 0xFEE00800, Base address: 0xFEE00000, State: Enabled, Type: Application Processor
[SMP] Started AP 13
[SMP] Starting APIC ID: 14
[SMP] Starting Core.
[APIC] Startup: 0xFEE00800, Base address: 0xFEE00000, State: Enabled, Type: Application Processor
[SMP] Started AP 14
[SMP] Starting APIC ID: 15
[SMP] Starting Core.
[APIC] Startup: 0xFEE00800, Base address: 0xFEE00000, State: Enabled, Type: Application Processor
[SMP] Started AP 15
)";

static void DumpImage(const char *filename, KernelFrameBuffer framebuffer, km::Canvas canvas) {
    uint8_t channels = 3;
    KernelFrameBuffer rgbFrame = {
        .width = framebuffer.width,
        .height = framebuffer.height,
        .pitch = framebuffer.width * channels,
        .bpp = uint16_t(8u * channels),
        .redMaskSize = 8,
        .redMaskShift = 16,
        .greenMaskSize = 8,
        .greenMaskShift = 8,
        .blueMaskSize = 8,
        .blueMaskShift = 0,
    };

    std::unique_ptr<uint8_t[]> rgbData(new uint8_t[rgbFrame.size()]);
    km::Canvas rgbCanvas(rgbFrame, rgbData.get());

    ASSERT_EQ(rgbCanvas.bytesPerPixel(), channels);

    for (uint64_t x = 0; x < framebuffer.width; x++) {
        for (uint64_t y = 0; y < framebuffer.height; y++) {
            km::Pixel pixel = canvas.read(x, y);

            rgbCanvas.write(x, y, pixel);
        }
    }

    int err = stbi_write_bmp(filename, rgbCanvas.width(), rgbCanvas.height(), channels, rgbCanvas.address());
    ASSERT_NE(err, 0) << "Failed to write image: " << filename;
}

TEST(TerminalTest, WriteText) {
    KernelFrameBuffer framebuffer = {
        .width = 1920,
        .height = 1080,
        .pitch = 5120,
        .bpp = 32,
        .redMaskSize = 8,
        .redMaskShift = 16,
        .greenMaskSize = 8,
        .greenMaskShift = 8,
        .blueMaskSize = 8,
        .blueMaskShift = 0,
    };

    std::unique_ptr<uint8_t[]> displayData(new uint8_t[framebuffer.size()]);
    memset(displayData.get(), 0, framebuffer.size());

    framebuffer.address = displayData.get();

    km::Canvas display(framebuffer, displayData.get());

    km::DirectTerminal terminal(display);

    terminal.print("A");

    DumpImage("terminal.bmp", framebuffer, display);

    uint64_t whiteCount = 0;

    for (uint64_t x = 0; x < framebuffer.width; x++) {
        for (uint64_t y = 0; y < framebuffer.height; y++) {
            km::Pixel displayPixel = display.read(x, y);

            // pixel must either be black or white

            km::Pixel black { 0, 0, 0 };
            km::Pixel white { 255, 255, 255 };

            if (displayPixel == white) {
                whiteCount++;
            }

            // all pixels below the text must be black
            if (y > 16) {
                ASSERT_TRUE(displayPixel == black)
                    << "Pixel in display at (" << x << ", " << y << ") is (" << uint32_t(displayPixel.r) << ", " << uint32_t(displayPixel.g) << ", " << uint32_t(displayPixel.b) << ")";
            } else {
                ASSERT_TRUE(displayPixel == black || displayPixel == white)
                    << "Pixel in display at (" << x << ", " << y << ") is (" << uint32_t(displayPixel.r) << ", " << uint32_t(displayPixel.g) << ", " << uint32_t(displayPixel.b) << ")";
            }
        }
    }

    // at least 100 pixels must be white, stops the test from passing if the terminal is not working
    ASSERT_TRUE(whiteCount > 100);
}
