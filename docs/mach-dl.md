# Mach-DL shared library file specification

Shared libraries have often been bundled in the same format as executables like ELF. this often leads to bloated files and hacked on features, to rememdy this shared libraries should be kept in their own format to simplify parsing and increase certainty on what a file is used for.

## Header

```c
typedef struct {
    file_header_t universal;
} machdl_header_t;