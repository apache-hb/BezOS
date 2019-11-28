# Jeff BezOS
welcome to the source code for Jeff BezOS. The first operating system built for Jeff.

## What?
* operating system
* focus on simple system apis
* written in C

## Is not
* massivley compatible
* supporting anything other than x86_64
* designed to support legacy systems
* designed to be interoperable

### Building

#### Windows
* install mingw
* install meson
* install i386-elf-gcc crosstools
* install qemu
* run `setup.bat`

#### Mac
* install brew
* install meson
* install i386-elf-gcc crosstools
* install qemu
* run `setup.command`

#### Linux
* install meson
* install crosstools
* install qemu
* run `setup.sh`