#pragma once

#include "memory.hpp"
#include "std/static_string.hpp"

#include <stdint.h>

struct SmBiosHeader64 {
    char anchor[5];
    uint8_t checksum;
    uint8_t length;
    uint8_t major;
    uint8_t minor;
    uint8_t revision;
    uint8_t entryRevision;
    uint8_t reserved0[1];
    uint32_t tableSize;
    uint64_t tableAddress;
};

struct SmBiosHeader32 {
    char anchor0[4];
    uint8_t checksum0;

    uint8_t length;
    uint8_t major;
    uint8_t minor;
    uint16_t tableSize;
    uint8_t entryRevision;
    uint8_t reserved0[5];

    char anchor1[5];
    uint8_t checksum1;

    uint16_t tableLength;
    uint32_t tableAddress;
    uint16_t entryCount;
    uint8_t bcdRevision;
};

struct SmBiosEntryHeader {
    uint8_t type;
    uint8_t length;
    uint16_t handle;
};

struct SmBiosFirmwareInfo {
    SmBiosEntryHeader header;
    uint8_t vendor;
    uint8_t version;
    uint16_t start;
    uint8_t build;
    uint8_t romSize;
    uint64_t characteristics;
    uint8_t reserved0[1];
};

struct SmBiosSystemInfo {
    SmBiosEntryHeader header;
    uint8_t manufacturer;
    uint8_t productName;
    uint8_t version;
    uint8_t serialNumber;
    uint8_t uuid[16];
    uint8_t wakeUpType;
    uint8_t skuNumber;
    uint8_t family;
};

struct PlatformInfo {
    stdx::StaticString<64> vendor;
    stdx::StaticString<64> version;

    stdx::StaticString<64> manufacturer;
    stdx::StaticString<64> product;
    stdx::StaticString<64> serial;

    bool isOracleVirtualBox() const;
};

PlatformInfo KmGetPlatformInfo(const KernelLaunch& launch, km::SystemMemory& memory);
