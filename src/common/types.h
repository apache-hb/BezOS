#ifndef TYPES_H
#define TYPES_H

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

typedef signed char int8;
typedef signed short int16;
typedef signed int int32;
typedef signed long long int64;

typedef uint8 byte;
typedef uint16 word;
typedef uint32 dword;
typedef uint64 qword;

template<uint64 N>
using pad = byte[N];

#define PACKED(name, ...) typedef struct __attribute__((packed)) __VA_ARGS__ name

#endif 
