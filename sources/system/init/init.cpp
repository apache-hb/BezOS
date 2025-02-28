#include <bezos/facility/device.h>
#include <bezos/facility/threads.h>
#include <bezos/facility/fs.h>
#include <bezos/facility/vmem.h>

#include <bezos/subsystem/hid.h>
#include <bezos/subsystem/ddi.h>

#include <flanterm.h>
#include <backends/fb.h>

#include <string.h>
#include <algorithm>
#include <iterator>

static constexpr size_t kBufferSize = 16;
static constexpr char kKeyboardDevicePath[] = OS_DEVICE_PS2_KEYBOARD;

template<size_t N>
static void DebugLog(const char (&message)[N]) {
    OsDebugLog(message, message + N - 1);
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

    OsDebugLog(ptr, buffer + sizeof(buffer));
}

template<size_t N>
static void Assert(bool condition, const char (&message)[N]) {
    if (!condition) {
        DebugLog(message);

        while (true) {
            OsThreadYield();
        }
    }
}

template<size_t N>
static void AssertOsSuccess(OsStatus status, const char (&message)[N]) {
    if (status != OsStatusSuccess) {
        Assert(false, message);
    }
}

#define ASSERT_OS_SUCCESS(expr) AssertOsSuccess(expr, "Assertion failed: " #expr " != OsStatusSuccess")
#define ASSERT(expr) Assert(expr, #expr)

template<size_t N>
OsAnyPointer VirtualAllocate(OsSize size, OsMemoryAccess access, const char (&name)[N]) {
    OsAddressSpaceCreateInfo createInfo = {
        .NameFront = std::begin(name),
        .NameBack = std::end(name) - 1,

        .Size = size,
        .Access = access,
    };

    OsAddressSpaceHandle handle{};
    ASSERT_OS_SUCCESS(OsAddressSpaceCreate(createInfo, &handle));

    OsAddressSpaceInfo stat{};
    ASSERT_OS_SUCCESS(OsAddressSpaceStat(handle, &stat));

    return stat.Base;
}

#define UTIL_NOCOPY(cls) \
    cls(const cls &) = delete; \
    cls &operator=(const cls &) = delete;

#define UTIL_NOMOVE(cls) \
    cls(cls &&) = delete; \
    cls &operator=(cls &&) = delete;

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
        }

        return status;
    }

    bool IsBufferEmpty() const {
        return mBufferIndex == mBufferCount;
    }

public:
    KeyboardDevice() {
        OsDeviceCreateInfo createInfo = {
            .NameFront = std::begin(kKeyboardDevicePath),
            .NameBack = std::end(kKeyboardDevicePath) - 1,

            .InterfaceGuid = kOsHidClassGuid,
        };

        ASSERT_OS_SUCCESS(OsDeviceOpen(createInfo, &mDevice));
    }

    ~KeyboardDevice() {
        ASSERT_OS_SUCCESS(OsDeviceClose(mDevice));
    }

    bool NextEvent(OsHidEvent& event, OsInstant timeout = OS_TIMEOUT_INFINITE) {
        if (IsBufferEmpty()) {
            if (FillBuffer(timeout)) {
                return false;
            }
        }

        if (IsBufferEmpty()) {
            return false;
        }

        event = mBuffer[mBufferIndex++];
        return true;
    }
};

static constexpr char kDisplayDevicePath[] = OS_DEVICE_DDI_RAMFB;

class VtDisplay {
    UTIL_NOCOPY(VtDisplay)
    UTIL_NOMOVE(VtDisplay)

    OsDeviceHandle mDevice;
    void *mAddress;
    flanterm_context *mContext;

public:
    VtDisplay() {
        OsDeviceCreateInfo createInfo = {
            .NameFront = std::begin(kDisplayDevicePath),
            .NameBack = std::end(kDisplayDevicePath) - 1,

            .InterfaceGuid = kOsDisplayClassGuid,
        };

        ASSERT_OS_SUCCESS(OsDeviceOpen(createInfo, &mDevice));

        OsDdiGetCanvas canvas{};
        ASSERT_OS_SUCCESS(OsDeviceCall(mDevice, eOsDdiGetCanvas, &canvas));

        OsDdiDisplayInfo display{};
        ASSERT_OS_SUCCESS(OsDeviceCall(mDevice, eOsDdiInfo, &display));

        mAddress = canvas.Canvas;
        ASSERT(mAddress != nullptr);

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

        ASSERT(mContext != nullptr);
    }

    ~VtDisplay() {
        ASSERT_OS_SUCCESS(OsDeviceClose(mDevice));
    }

    void Clear() {
        OsDdiFill fill = { .R = 100, .G = 100, .B = 100 };
        ASSERT_OS_SUCCESS(OsDeviceCall(mDevice, eOsDdiFill, &fill));
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

extern "C" size_t strlen(const char *str) {
    size_t len = 0;
    while (*str++) {
        len++;
    }
    return len;
}

extern "C" void *memset(void *ptr, int value, size_t num) {
    unsigned char *begin = static_cast<unsigned char *>(ptr);
    unsigned char *end = begin + num;

    for (unsigned char *it = begin; it != end; ++it) {
        *it = static_cast<unsigned char>(value);
    }

    return ptr;
}

extern "C" void *memcpy(void *dest, const void *src, size_t num) {
    unsigned char *d = static_cast<unsigned char *>(dest);
    const unsigned char *s = static_cast<const unsigned char *>(src);

    for (size_t i = 0; i < num; i++) {
        d[i] = s[i];
    }

    return dest;
}

template<size_t N>
static OsStatus OpenFile(const char (&path)[N], OsFileHandle *OutHandle) {
    const char *begin = path;
    const char *end = path + N - 1;

    return OsFileOpen(begin, end, eOsFileRead, OutHandle);
}

extern "C" bool isprint(int c) {
    return c >= 0x20 && c <= 0x7E;
}

extern "C" bool isalnum(int c) {
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

extern "C" int isalpha(int c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

extern "C" int toupper(int c) {
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 'A';
    }

    return c;
}

extern "C" int tolower(int c) {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 'a';
    }

    return c;
}

extern "C" int strcmp(const char *lhs, const char *rhs) {
    while (*lhs && *rhs && *lhs == *rhs) {
        lhs++;
        rhs++;
    }

    return *lhs - *rhs;
}

static void Prompt(VtDisplay& display) {
    display.WriteString("root@localhost: ");
}

// rdi: first argument
// rsi: last argument
// rdx: reserved
extern "C" [[noreturn]] void ClientStart(const void*, const void*, uint64_t) {
    OsFileHandle Handle = OS_FILE_INVALID;
    ASSERT_OS_SUCCESS(OpenFile("Users\0Guest\0motd.txt", &Handle));

    char buffer[256];
    uint64_t read = UINT64_MAX;
    ASSERT_OS_SUCCESS(OsFileRead(Handle, std::begin(buffer), std::end(buffer), &read));

    VtDisplay display{};

    display.WriteString(buffer, buffer + read);

    Prompt(display);

    char text[0x1000];
    size_t index = 0;

    KeyboardDevice keyboard{};
    OsHidEvent event{};
    while (true) {
        if (!keyboard.NextEvent(event)) {
            continue;
        }

        //
        // We only care about key down events for shell input.
        //
        if (event.Type != eOsHidEventKeyDown) {
            continue;
        }

        if (event.Body.Key.Key == eKeyReturn) {
            if (index == 0) {
                display.WriteString("\n");
                Prompt(display);
                continue;
            }

            size_t oldIndex = index;
            text[index] = '\0';
            index = 0;

            display.WriteString("\n");

            if (strcmp(text, "exit") == 0) {
                break;
            } else if (strcmp(text, "uname") == 0) {
                display.WriteString("BezOS localhost 0.0.1 amd64\n");
                Prompt(display);
                continue;
            }

            display.WriteString("Unknown command: ");
            display.WriteString(text, text + oldIndex);
            display.WriteString("\n");

            Prompt(display);
        } else if (event.Body.Key.Key == eKeyDelete) {
            if (index > 0) {
                index--;
                display.WriteString("\b \b");
            }
        } else if (isalpha(event.Body.Key.Key)) {
            char c = event.Body.Key.Key;
            if (event.Body.Key.Modifiers & eModifierShift) {
                c = toupper(c);
            } else {
                c = tolower(c);
            }

            text[index++] = c;
            display.WriteChar(c);
        } else if (isprint(event.Body.Key.Key)) {
            text[index++] = event.Body.Key.Key;
            display.WriteChar(event.Body.Key.Key);
        }
    }

    DebugLog("Shell exited\n");
    while (true) { }
    __builtin_unreachable();
}
