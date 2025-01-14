#ifndef SYSTEM_GDT_H
#define SYSTEM_GDT_H

#define GDT_NULL 0
#define GDT_16BIT_CODE 1
#define GDT_16BIT_DATA 2
#define GDT_32BIT_CODE 3
#define GDT_32BIT_DATA 4
#define GDT_64BIT_CODE 5
#define GDT_64BIT_DATA 6
#define GDT_64BIT_USER_CODE 7
#define GDT_64BIT_USER_DATA 8

#define GDT_COUNT 9

#define GDT_TSS0 9
#define GDT_TSS1 10

#define GDT_TSS_COUNT 11

#endif /* SYSTEM_GDT_H */
