#include <bezos/facility/device.h>
#include <bezos/facility/thread.h>
#include <bezos/facility/node.h>
#include <bezos/facility/vmem.h>
#include <bezos/facility/debug.h>

#include <bezos/subsystem/hid.h>
#include <bezos/subsystem/ddi.h>
#include <bezos/subsystem/identify.h>
#include <bezos/subsystem/fs.h>

#include <bezos/start.h>

#include <flanterm.h>
#include <backends/fb.h>

#include <ctype.h>

#include <iterator>

#define UTIL_NOCOPY(cls) \
    cls(const cls &) = delete; \
    cls &operator=(const cls &) = delete;

#define UTIL_NOMOVE(cls) \
    cls(cls &&) = delete; \
    cls &operator=(cls &&) = delete;

static constexpr size_t kBufferSize = 16;
static constexpr char kKeyboardDevicePath[] = OS_DEVICE_PS2_KEYBOARD;

template<size_t N>
static void DebugLog(const char (&message)[N]) {
    OsDebugMessage(eOsLogDebug, message);
}

template<typename T>
static void DebugLog(T number) {
    char buffer[32];
    char *ptr = buffer + sizeof(buffer);
    *--ptr = '\0';

    if (number == 0) {
        *--ptr = '0';
    } else {
        while (number != 0) {
            *--ptr = '0' + (number % 10);
            number /= 10;
        }
    }

    OsDebugMessage({ ptr, buffer + sizeof(buffer), eOsLogDebug });
}

template<size_t N>
static void Assert(bool condition, const char (&message)[N]) {
    if (!condition) {
        DebugLog(message);

        abort();
    }
}

template<size_t N>
static void AssertOsSuccess(OsStatus status, const char (&message)[N]) {
    if (status != OsStatusSuccess) {
        DebugLog(message);
        DebugLog("\nStatus: ");
        DebugLog(status);
        abort();
    }
}

#define ASSERT_OS_SUCCESS(expr) AssertOsSuccess(expr, "tty.elf: Assertion failed: " #expr " != OsStatusSuccess")
#define ASSERT(expr) Assert(expr, #expr)

static constexpr char kDisplayDevicePath[] = OS_DEVICE_DDI_RAMFB;

class KeyboardDevice {
    UTIL_NOCOPY(KeyboardDevice)
    UTIL_NOMOVE(KeyboardDevice)

    OsDeviceHandle mDevice;
    OsHidEvent mBuffer[kBufferSize];
    uint32_t mBufferIndex = 0;
    uint32_t mBufferCount = 0;

    OsStatus FillBuffer(OsInstant timeout) {
        OsSize read = 0;
        OsDeviceReadRequest request = {
            .BufferFront = std::begin(mBuffer),
            .BufferBack = std::end(mBuffer),
            .Timeout = timeout,
        };

        OsStatus status = OsDeviceRead(mDevice, request, &read);

        if (read != 0) {
            ASSERT(read % sizeof(OsHidEvent) == 0);

            mBufferIndex = 0;
            mBufferCount = read / sizeof(OsHidEvent);
        } else {
            return OsStatusEndOfFile;
        }

        return status;
    }

    bool IsBufferEmpty() const {
        return mBufferIndex == mBufferCount;
    }

public:
    KeyboardDevice() {
        OsDeviceCreateInfo createInfo = {
            .Path = OsMakePath(kKeyboardDevicePath),

            .InterfaceGuid = kOsHidClassGuid,
        };

        ASSERT_OS_SUCCESS(OsDeviceOpen(createInfo, &mDevice));
    }

    ~KeyboardDevice() {
        ASSERT_OS_SUCCESS(OsDeviceClose(mDevice));
    }

    bool NextEvent(OsHidEvent *event, OsInstant timeout = OS_TIMEOUT_INFINITE) {
        if (IsBufferEmpty()) {
            if (FillBuffer(timeout)) {
                return false;
            }
        }

        if (IsBufferEmpty()) {
            return false;
        }

        *event = mBuffer[mBufferIndex++];
        return true;
    }
};

class VtDisplay {
    UTIL_NOCOPY(VtDisplay)
    UTIL_NOMOVE(VtDisplay)

    OsDeviceHandle mDevice;
    void *mAddress;
    flanterm_context *mContext;

    static void ft_free(void *ptr, size_t) {
        free(ptr);
    }

public:
    VtDisplay() {
        DebugLog("hhhh");
        OsDeviceCreateInfo createInfo = {
            .Path = OsMakePath(kDisplayDevicePath),
            .InterfaceGuid = kOsDisplayClassGuid,
            .Flags = eOsDeviceOpenExisting,
        };

        DebugLog("Opening display");

        ASSERT_OS_SUCCESS(OsDeviceOpen(createInfo, &mDevice));

        DebugLog("aaaa");

        OsDdiGetCanvas canvas{};
        ASSERT_OS_SUCCESS(OsDeviceInvoke(mDevice, eOsDdiGetCanvas, &canvas, sizeof(canvas)));

        DebugLog("bbb");

        OsDdiDisplayInfo display{};
        ASSERT_OS_SUCCESS(OsDeviceInvoke(mDevice, eOsDdiInfo, &display, sizeof(display)));

        DebugLog("ccc");

        mAddress = canvas.Canvas;
        ASSERT(mAddress != nullptr);

        DebugLog("ddd");

        mContext = flanterm_fb_init(
            nullptr, nullptr,
            (uint32_t*)mAddress, display.Width, display.Height, display.Stride,
            display.RedMaskSize, display.RedMaskShift,
            display.GreenMaskSize, display.GreenMaskShift,
            display.BlueMaskSize, display.BlueMaskShift,
            nullptr,
            nullptr, nullptr,
            nullptr, nullptr,
            nullptr, nullptr,
            nullptr, 0, 0, 0,
            0, 0,
            0
        );

        DebugLog("eee");

        ASSERT(mContext != nullptr);
    }

    ~VtDisplay() {
        ASSERT_OS_SUCCESS(OsDeviceClose(mDevice));
    }

    void Clear() {
        OsDdiFill fill = { .R = 100, .G = 100, .B = 100 };
        ASSERT_OS_SUCCESS(OsDeviceInvoke(mDevice, eOsDdiFill, &fill, sizeof(fill)));
    }

    void WriteChar(char c) {
        char buffer[1] = { c };
        flanterm_write(mContext, buffer, 1);
    }

    void WriteString(const char *begin, const char *end) {
        flanterm_write(mContext, begin, end - begin);
    }

    template<size_t N>
    void WriteString(const char (&str)[N]) {
        WriteString(str, str + N - 1);
    }
};

class StreamDevice {
    OsDeviceHandle mDevice;

public:
    StreamDevice(OsPath path) {
        OsDeviceCreateInfo createInfo {
            .Path = path,
            .InterfaceGuid = kOsStreamGuid,
            .Flags = eOsDeviceOpenAlways,
        };

        ASSERT_OS_SUCCESS(OsDeviceOpen(createInfo, &mDevice));
    }

    ~StreamDevice() {
        ASSERT_OS_SUCCESS(OsDeviceClose(mDevice));
    }

    OsStatus Write(const char *front, const char *back) {
        OsDeviceWriteRequest request {
            .BufferFront = front,
            .BufferBack = back,
            .Timeout = OS_TIMEOUT_INFINITE,
        };

        OsSize written = 0;
        return OsDeviceWrite(mDevice, request, &written);
    }

    OsStatus Read(char *front, char *back, size_t *size) {
        OsDeviceReadRequest request {
            .BufferFront = front,
            .BufferBack = back,
            .Timeout = OS_TIMEOUT_INSTANT,
        };

        return OsDeviceRead(mDevice, request, size);
    }
};

static char ConvertVkToAscii(OsHidKeyEvent event) {
    switch (event.Key) {
    case eKeyDivide: return '/';
    case eKeyMultiply: return '*';
    case eKeySubtract: return '-';
    case eKeyAdd: return '+';
    case eKeyPeriod: return '.';
    case eKeyReturn: return '\n';
    case eKeySpace: return ' ';
    case eKeyTab: return '\t';
    case eKeyDelete: return '\b';
    default: break;
    }

    if (isalpha(event.Key)) {
        return (event.Modifiers & eModifierShift) ? toupper(event.Key) : tolower(event.Key);
    }

    if (isdigit(event.Key)) {
        return (event.Modifiers & eModifierShift) ? ")!@#$%^&*("[event.Key - '0'] : event.Key;
    }

    return '\0';
}

OS_EXTERN
[[noreturn, gnu::force_align_arg_pointer]]
void ClientStart(const struct OsClientStartInfo *) {
    DebugLog("TTY0: Starting TTY0...");

    VtDisplay display{};

    DebugLog("fff");

    KeyboardDevice keyboard{};

    DebugLog("ggg");

    OsHidEvent event{};
    char buffer[256]{};

    DebugLog("TTY0: Starting TTY0...");

    StreamDevice ttyin{OsMakePath("Devices\0Terminal\0TTY0\0Input")};
    StreamDevice ttyout{OsMakePath("Devices\0Terminal\0TTY0\0Output")};

    display.WriteString("Device '/Devices/Terminal/TTY0' is ready.\n");

    while (true) {
        while (keyboard.NextEvent(&event)) {
            if (event.Type == eOsHidEventKeyDown) {
                if (char c = ConvertVkToAscii(event.Body.Key)) {
                    char buffer[1] = { c };
                    ttyin.Write(std::begin(buffer), std::end(buffer));
                }
            }
        }

        size_t read = 0;
        while (ttyout.Read(std::begin(buffer), std::end(buffer), &read) == OsStatusSuccess && read != 0) {
            display.WriteString(buffer, buffer + read);
        }

        OsThreadYield();
    }
}
