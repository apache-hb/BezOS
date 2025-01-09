#include <gtest/gtest.h>

#include <cstdlib>

#include <stb_image_write.h>

#include "display.hpp"

static constexpr stdx::StringView kText = R"(
| /SYS/PCI/00.1F.00 | Programmable         | 0
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
[INIT] PS/2 controller: Ok
[INIT] PS/2 channel 1: Present
[INIT] PS/2 channel 2: Present
[PS2] Keyboard scancode: 0xFA
[PS2] Keyboard scancode: 0xFA
)";

static void DumpImage(const char *filename, KernelFrameBuffer framebuffer, km::Canvas canvas) {
    std::unique_ptr<uint8_t[]> data(new uint8_t[framebuffer.width * framebuffer.height * 3]);
    KernelFrameBuffer rgbFramebuffer = {
        .width = framebuffer.width,
        .height = framebuffer.height,
        .pitch = framebuffer.width * 3,
        .bpp = 24,
        .redMaskSize = 8,
        .redMaskShift = 16,
        .greenMaskSize = 8,
        .greenMaskShift = 8,
        .blueMaskSize = 8,
        .blueMaskShift = 0,
    };

    km::Canvas rgbCanvas{rgbFramebuffer, data.get()};

    for (uint64_t x = 0; x < framebuffer.width; x++) {
        for (uint64_t y = 0; y < framebuffer.height; y++) {
            km::Pixel pixel = canvas.read(x, y);
            rgbCanvas.write(x, y, pixel);
        }
    }

    stbi_write_bmp(filename, framebuffer.width, framebuffer.height, 3, data.get());
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

    terminal.print(kText);

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

            ASSERT_TRUE(displayPixel == black || displayPixel == white)
                << "Pixel in display at (" << x << ", " << y << ") is (" << displayPixel.r << ", " << displayPixel.g << ", " << displayPixel.b << ")";
        }
    }

    // at least 100 pixels must be white, stops the test from passing if the terminal is not working
    ASSERT_TRUE(whiteCount > 100);
}
