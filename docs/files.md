# BezOS file specification

The BezOS file specification aims to simplify and unify core parts of every file format to improve performance and simplify user experiences.

## Headers

all non human readable files should begin with 32 bits of file magic and a 32 bit wide packed version.

```c
typedef struct {
    u32 magic;

    // version
    u8 major;
    u8 minor;
    u16 patch;
} file_header_t;
```

1. It is recommended that `magic` be a 3 or 4 byte human readable string containing only printable characters
2. It is recommended that versioning standards are followed to make the OS do less work when deciding which version of an application it should use on a file
3. `patch` should be incremented with every bugfix
4. `minor` should be incremented whenever a feature is added that does break backwards compatibility
5. `major` should be incremented whenever the file format changes to an extent that a previous version would be unable to use the file