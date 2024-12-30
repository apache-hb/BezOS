# Jeff BezOS
I thought it would be funny

## Building
1. install `clang` (preferrably 9/10), `nasm`, `python3.7`
2. execute
```sh
meson build --cross-file x64-clang-cross.ini
ninja -C build
```
1. kernel will be in `build/boot/<boot>/bezos.bin`

## Running

### Bios
1. install `qemu`
2. run with `qemu.bat build\boot\<boot>\bezos.bin`

## Mentions
1. Marley on the osdev discord for being incredibly clever and knowledgable
2. osdev wiki for (sometimes) good reference
3. all the random github os projects i've browsed to figure out how to do things
