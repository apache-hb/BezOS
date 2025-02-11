# Jeff BezOS
I thought it would be funny

## Building
1. install `clang` 20 and `meson`
2. execute
```sh
./setup.sh build
make
```
3. kernel will be at `install/bezos-limine.iso` or `install/bezos-hyper.iso`

## Running

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
