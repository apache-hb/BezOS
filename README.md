# Jeff BezOS
I thought it would be funny

## Building
1. install `clang` and `meson`
2. execute
```sh
source ./setup.sh
repobld kernel
```
3. kernel will be at `install/image/bezos-limine.iso` or `install/image/bezos-hyper.iso`

## Running

### Tests
```
make check
```

### On qemu
```sh
./qemu.sh
```

### On other emulators (assumes wsl2 inside a windows host)
```sh
make hyperv
make vbox
make vmware
```

## Mentions
1. Marley on the osdev discord for being incredibly clever and knowledgable
2. osdev wiki for (sometimes) good reference
3. all the random github os projects i've browsed to figure out how to do things
