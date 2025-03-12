#pragma once

#include <bezos/handle.h>

#include <bezos/facility/device.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OS_SERIAL_NAME_MAX 64
#define OS_VENDOR_NAME_MAX 64
#define OS_FIELD_NAME_MAX 64
#define OS_INTERFACE_NAME_MAX 64

/// @brief The GUID for device capability identification.
///
/// All vfs nodes implement this interface and this should be used
/// to query the capabilities and information about the device.
OS_DEFINE_GUID(kOsIdentifyGuid, 0xba72898c, 0xf2d8, 0x11ef, 0x8520, 0x0b9e4d54e5a1);

enum {
    eOsIdentifyInfo          = UINT64_C(0),
    eOsIdentifyInterfaceList = UINT64_C(1),
    eOsIdentifyPropertyList  = UINT64_C(2),
};

struct OsIdentifyInfo {
    /// @brief The device name to display to the user.
    /// @example "Samsung SSD 990 EVO 2TB"
    OsUtf8Char DisplayName[OS_FIELD_NAME_MAX];

    /// @brief The device model name.
    /// @example "MZ-V9E2T0B/AM"
    OsUtf8Char Model[OS_FIELD_NAME_MAX];

    /// @brief The device serial number.
    /// @example "S7M4NS0X202344T"
    OsUtf8Char Serial[OS_SERIAL_NAME_MAX];

    /// @brief The name of the device vendor.
    /// @example "Samsung Electronics"
    OsUtf8Char DeviceVendor[OS_VENDOR_NAME_MAX];

    /// @brief The device firmware revision.
    /// @example "0B2QKXJ7"
    OsUtf8Char FirmwareRevision[OS_FIELD_NAME_MAX];

    /// @brief The name of the device driver vendor.
    /// @example "Microsoft Corporation"
    OsUtf8Char DriverVendor[OS_VENDOR_NAME_MAX];

    /// @brief The version of the device driver.
    /// @example "1.0.0"
    OsVersionTag DriverVersion;
};

struct OsIdentifyInterfaceList {
    uint32_t InterfaceCount;
    struct OsGuid InterfaceGuids[];
};

struct OsIdentifyPropertyEntry {
    uint32_t NameStart;
    uint32_t DataStart;
};

struct OsIdentifyPropertyList {
    uint32_t PropertyCount;
    uint32_t BufferSize;

    OsByte Buffer[] OS_COUNTED_BY(BufferSize);
};

/// @brief Identify the interfaces supported by the device.
///
/// This function is used to query the list of interfaces supported by the device.
/// The list of interfaces is returned in the `OutList` parameter.
///
/// @pre The @p OutList parameter must be a valid pointer to a buffer that can hold the list of interfaces.
/// @pre @p OutList->InterfaceCount must be set to the number of interfaces that can be stored in the buffer.
/// @post On success the @p OutList->InterfaceCount will be set to the number of interfaces that were stored in the buffer.
///
/// @note If the buffer is too small to store the list of interfaces, the function will return @a OsStatusMoreData
///       and @p OutList->InterfaceCount will be set to the number of available interfaces.
///
/// @param Handle The handle to the device.
/// @param OutList The buffer to store the list of interfaces.
///
/// @return Returns an error code on failure.
inline OsStatus OsInvokeIdentifyInterfaceList(OsDeviceHandle Handle, struct OsIdentifyInterfaceList *OutList) {
    return OsDeviceCall(Handle, eOsIdentifyInterfaceList, OutList, sizeof(struct OsIdentifyInterfaceList) + (OutList->InterfaceCount * sizeof(struct OsGuid)));
}

/// @brief Query device information.
///
/// This function is used to query the information about the device.
///
/// @param Handle The handle to the device.
/// @param OutInfo The buffer to store the device information.
///
/// @return Returns an error code on failure.
inline OsStatus OsInvokeIdentifyDeviceInfo(OsDeviceHandle Handle, struct OsIdentifyInfo *OutInfo) {
    return OsDeviceCall(Handle, eOsIdentifyInfo, OutInfo, sizeof(struct OsIdentifyInfo));
}

inline OsStatus OsInvokeIdentifyPropertyList(OsDeviceHandle Handle, struct OsIdentifyPropertyList *OutList) {
    return OsDeviceCall(Handle, eOsIdentifyPropertyList, OutList, sizeof(struct OsIdentifyPropertyList) + OutList->BufferSize);
}

#ifdef __cplusplus
}
#endif
