# Jeff BezOS
I thought it would be funny

## Building
1. install `clang` (preferrably 9/10), `nasm`, `python3.7`
2. execute 
```sh
git submodule update --init
./build.py <target> [options...]
# target is any one of [bios | uefi | bootboot | multiboot1 | multiboot2 | bootboot | stivale]
# options can be any of [release]
```
3. kernel will be in either `bios/bezos.bin` or `uefi/BOOTX64.EFI`

## Running

### Bios
1. install `qemu`
2. run with `qemu-system-x86_64 -drive file=bezos.bin,format=raw`

### UEFI
1. who knows, uefi is horrible and a pain to test so as far as im concerned it doesnt exist

## Mentions
1. Marley on the osdev discord for being incredibly clever and knowledgable 
2. osdev wiki for (sometimes) good reference
3. all the random github os projects i've browsed to figure out how to do things