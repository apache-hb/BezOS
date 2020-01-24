#ifndef UTILS_CONVERT_H
#define UTILS_CONVERT_H 1

#define PAGE_ALIGN(val) ((val + 0x1000 - 1) & -0x1000)

#define KILOBYTE(val) (val * 1024)
#define MEGABYTE(val) (KILOBYTE(val) * 1024)
#define GIGABYTE(val) (MEGABYTE(val) * 1024)
#define TERABYTE(val) (GIGABYTE(val) * 1024)
#define PETABYTE(val) (TERABYTE(val) * 1024)

#endif // UTILS_CONVERT_H
