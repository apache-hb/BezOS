# Mach-BZ executable file specification

The Mach-BZ file specification aims to create an executable format that fills the needs of modern users and simplifies the developer process.

## Header

```c
typedef struct {
    file_header_t universal;
} machbz_header_t;