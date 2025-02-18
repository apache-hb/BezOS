#include <bezos/facility/device.h>
#include <bezos/facility/threads.h>
#include <bezos/facility/fs.h>

#include <bezos/subsystem/hid.h>
#include <bezos/subsystem/ddi.h>

#include <cstring>
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
    if (OsStatusError(status)) {
        Assert(false, message);
    }
}

#define ASSERT_OS_SUCCESS(expr) AssertOsSuccess(expr, "Assertion failed: " #expr " != OsStatusSuccess")
#define ASSERT(expr) Assert(expr, #expr)

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
static constexpr size_t kDisplayWidth = 80;
static constexpr size_t kDisplayHeight = 25;
static constexpr char kDisplayDevicePath[] = OS_DEVICE_DDI_RAMFB;

class VtDisplay {
    UTIL_NOCOPY(VtDisplay)
    UTIL_NOMOVE(VtDisplay)

    OsDeviceHandle mDevice;
    char mTextBuffer[kDisplayWidth * kDisplayHeight];
    uint16_t mColumn = 0;
    uint16_t mRow = 0;

    void Scroll() {
        memmove(mTextBuffer, mTextBuffer + kDisplayWidth, kDisplayWidth * (kDisplayHeight - 1));
        memset(mTextBuffer + kDisplayWidth * (kDisplayHeight - 1), ' ', kDisplayWidth);
    }

public:
    VtDisplay() {
        OsDeviceCreateInfo createInfo = {
            .NameFront = std::begin(kDisplayDevicePath),
            .NameBack = std::end(kDisplayDevicePath) - 1,

            .InterfaceGuid = kOsDisplayClassGuid,
        };

        ASSERT_OS_SUCCESS(OsDeviceOpen(createInfo, &mDevice));

        Clear();
    }

    ~VtDisplay() {
        ASSERT_OS_SUCCESS(OsDeviceClose(mDevice));
    }

    void Clear() {
        std::fill(std::begin(mTextBuffer), std::end(mTextBuffer), ' ');
        mColumn = 0;
        mRow = 0;

        OsDdiFill fill = { .R = 100, .G = 100, .B = 100 };
        ASSERT_OS_SUCCESS(OsDeviceCall(mDevice, eOsDdiFill, &fill));
    }

    void WriteChar(char c) {

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

template<size_t N>
static OsStatus OpenFile(const char (&path)[N], OsFileHandle *OutHandle) {
    const char *begin = path;
    const char *end = path + N - 1;

    return OsFileOpen(begin, end, eOsFileRead, OutHandle);
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

    OsDebugLog(buffer, buffer + read);

    VtDisplay display{};

    display.Clear();

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

        DebugLog("Key down event received\n");
    }

    DebugLog("Shell exited\n");
    while (true) { }
    __builtin_unreachable();
}
