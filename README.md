# Jeff BezOS
I thought it would be funny

## Building
1. install `clang`, `nasm`, `meson>=0.52.0`, `ld`
2. execute (change cross file for targeting different platforms)
```sh
meson build --cross-file x86-64-cross.ini
cd build
ninja
```
3. kernel will be sitting in the build directory called `bezos.bin`

## Running
1. install `qemu-system-x86_64`
2. run with `qemu-system-x86_64 bezos.bin`

## Mentions
1. Marley on the osdev discord for being incredibly clever and knowledgable 
2. osdev wiki for good reference 
3. all the random github os projects i've browsed to figure out how to do things