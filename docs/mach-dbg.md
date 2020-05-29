# Mach-DBG debug symbol file specification

Debug symbols are commonly used by developers to track down bugs, but the end user rarely requires them. As such they should be stored in a seperate file to prevent accidental shipping of symbols that could lead to security flaws and bloated application packages.

## Header

```c
typedef struct {
    file_header_t universal;
} machdbg_header_t;
```