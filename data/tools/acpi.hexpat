struct RsdtHeader {
    char signature[4];
    u32 length;
    u8 revision;
    u8 checksum;
    char oemid[6];
    char tableId[8];
    u32 oemRevision;
    u32 creatorId;
    u32 creatorRevision;

    u8 data[length - 36];
};

fn pkg_length_byte_count(u8 lead) {
    return (lead & 0b11000000) >> 6;
};

struct PkgLength {
    u8 lead;

    if (pkg_length_byte_count(lead) != 0) {
        u8 data[pkg_length_byte_count(lead)];
    }
};

fn get_pkg_length(PkgLength length) {
    u32 result = length & 0b00111111;
    u8 bytes = pkg_length_byte_count(lead);
    for (u8 i = 0, i < bytes, i += 1) {
        result = result | length.data[i] << (((i + 1) * 8) - 4);
    }
    return result;
};
