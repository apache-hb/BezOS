#include <bezos/facility/device.h>
#include <bezos/facility/threads.h>
#include <bezos/facility/fs.h>

#include <bezos/subsystem/hid.h>
#include <bezos/subsystem/ddi.h>

#include <cstring>
#include <iterator>

static constexpr size_t kBufferSize = 16;
static constexpr char kKeyboardDevicePath[] = OS_DEVICE_PS2_KEYBOARD;

template<size_t N>
static void DebugLog(const char (&message)[N]) {
    OsDebugLog(message, message + N - 1);
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

    bool NextEvent(OsHidEvent& event, OsInstant timeout = 0) {
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

extern "C" size_t strlen(const char *str) {
    size_t len = 0;
    while (*str++) {
        len++;
    }
    return len;
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
    if (OsStatus _ = OpenFile("Users\0Guest\0motd.txt", &Handle)) {
        DebugLog("Failed to open file /Users/Guest/motd.txt");
        while (1) { }
        __builtin_unreachable();
    }

    char buffer[256];
    uint64_t read = UINT64_MAX;
    if (OsStatus _ = OsFileRead(Handle, buffer, buffer + sizeof(buffer), &read)) {
        DebugLog("Failed to read file /Users/Guest/motd.txt");
        while (1) { }
        __builtin_unreachable();
    }

    DebugLog("Reading file /Users/Guest/motd.txt");
    OsDebugLog(buffer, buffer + read);

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
}
