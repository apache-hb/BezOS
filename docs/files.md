# BezOS file specification

The BezOS file specification aims to simplify and unify core parts of every file format to improve performance and simplify user experiences.

## Headers

all non human readable files should begin with 32 bits of file magic and a 32 bit wide packed version.

```c
typedef struct {
    uint32_t magic;

    // version data
    uint8_t major;
    uint8_t minor;
    uint16_t patch;
} file_header_t;
```

1. `magic` can be any value between 0 and UINT32_MAX. It is recommended to treat this field as a 4 character string that only uses printable ascii characters and `\0` nul. But it is optional to do so.
2. It is recommended that versioning standards are followed to make the OS do less work when deciding which version of an application it should use on a file
3. `patch` should be incremented with every bugfix
4. `minor` should be incremented whenever a feature is added that does break backwards compatibility
5. `major` should be incremented whenever the file format changes to an extent that a previous version would be unable to use the file
