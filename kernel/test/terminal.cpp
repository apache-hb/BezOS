#include <gtest/gtest.h>

#include <cstdlib>

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
    std::unique_ptr<uint8_t[]> backbufferData(new uint8_t[framebuffer.size()]);
    memset(displayData.get(), 0, framebuffer.size());
    memset(backbufferData.get(), 0, framebuffer.size());

    framebuffer.address = displayData.get();

    km::Canvas display(framebuffer, displayData.get());

    km::Canvas backbuffer(display, backbufferData.get());

    km::BufferedTerminal terminal(display, backbuffer);

    terminal.print(kText);

    for (uint64_t x = 0; x < framebuffer.width; x++) {
        for (uint64_t y = 0; y < framebuffer.height; y++) {
            km::Pixel displayPixel = display.read(x, y);
            km::Pixel backbufferPixel = backbuffer.read(x, y);

            // pixel must either be black or white

            km::Pixel black { 0, 0, 0 };
            km::Pixel white { 255, 255, 255 };

            ASSERT_TRUE(displayPixel == black || displayPixel == white)
                << "Pixel in display at (" << x << ", " << y << ") is (" << displayPixel.r << ", " << displayPixel.g << ", " << displayPixel.b << ")";

            ASSERT_TRUE(backbufferPixel == black || backbufferPixel == white)
                << "Pixel in backbuffer at (" << x << ", " << y << ") is (" << backbufferPixel.r << ", " << backbufferPixel.g << ", " << backbufferPixel.b << ")";
        }
    }
}
