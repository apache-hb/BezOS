#!/usr/bin/env python3

#TODO: rewrite the build script again

import subprocess
import os
import sys
import shutil

uefi_src = [ 'src/boot/uefi/uefi.cpp' ]
bios_src = [ 'src/kernel/kmain.cpp', 'src/kernel/mm/mm.cpp']

release = False

def default_cxx_args():
    return [
        '-mno-sse',
        '-mno-sse2',
        '-mno-mmx',
        '-ffreestanding',
        '-fno-exceptions',
        '-fno-rtti',
        '-std=c++17',
        '-Isrc',
        '-Isrc/kernel',
        '-m64',
        '-Wall',
        '-Wextra',
        '-Werror'
    ] + [ '-O3' ] if release else []

def get_ovmf(path, srcs):
    subprocess.call(['wget', '--quiet', 'https://dl.bintray.com/no92/vineyard-binary/OVMF.fd', '-O', f'{path}/OVMF.fd'])
    exit(0)

def build_bios(path, srcs):
    obj = f'{path}/kernel.o'
    out = f'{path}/bezos.bin'
    elf = f'{path}/bezos.elf'
    link = 'src/boot/bios/bios.ld'
    cxx = 'clang++'
    ld = 'ld'
    cxx_args = [
        '-fno-addrsig',
        '-fno-builtin',
        '-fno-common',
        '-nostdlib',
        '-static',
        '-ffast-math',
        '-fdelete-null-pointer-checks',
        '-fno-threadsafe-statics',
        '-fno-stack-protector',
        '-mno-red-zone',
        '-fno-pic',
        '-pipe',
        '-fshort-wchar',
        '-mcmodel=kernel',
        '-static'
    ] + default_cxx_args()
    ld_args = [ '-nostdlib' ]
    asm_src = 'src/boot/bios/boot.asm'
    asm = 'nasm'
    asm_obj = f'{path}/boot.o'

    copy = 'objcopy'

    subprocess.call([asm, '-felf64', asm_src, '-o', asm_obj])
    subprocess.call([cxx] + cxx_args + ['-c'] + srcs + ['-o', obj])
    subprocess.call([ld] + ld_args + [asm_obj, obj, '-T' + link, '-o', elf])
    subprocess.call([copy, elf, out, '-R', '.ignore', '-O', 'binary'])

def build_uefi(path, srcs):
    obj = f'{path}/uefi.o'
    efi = f'{path}/BOOTX64.EFI'
    cxx = 'clang++'
    ld = 'lld-link'
    cxx_args = [ '--target=x86_64-pc-win32-coff', '-fno-stack-protector', '-fshort-wchar', '-mno-red-zone', '-Isubprojects', '-std=c++17' ] + default_cxx_args()
    ld_args = [ '-subsystem:efi_application', '-nodefaultlib', '-dll', '-entry:efi_main', obj, f'-out:{efi}' ]

    subprocess.call([cxx] + cxx_args + ['-c'] + srcs + ['-o', obj])
    subprocess.call([ld] + ld_args)

    img = f'{path}/bezos.img'

    subprocess.call(['dd', 'if=/dev/zero', f'of={img}', 'bs=1k', 'count=1440'])
    subprocess.call(['mformat', '-i', img, '-f', '1440', '::'])
    subprocess.call(['mmd', '-i', img, '::/EFI'])
    subprocess.call(['mmd', '-i', img, '::/EFI/BOOT'])
    subprocess.call(['mcopy', '-i', img, efi, '::/EFI/BOOT'])


builddir = 'build'

def unity(path, src):
    with open(f'{path}/unity.cpp', 'w+') as unity:
        for each in src:
            unity.write(f'#include <{os.path.abspath(each)}>\n')

    # we return a list for now because as we get more files we may need to split unity builds
    return [ f'{path}/unity.cpp' ]

class Target:
    def __init__(self, name, callback, desc = 'no description', src = []):
        self.name = name
        self.callback = callback
        self.desc = desc
        self.src = src

    def run(self):
        subdir = f'{builddir}/{self.name}'
        if not os.path.exists(subdir):
            os.mkdir(subdir)

        # if this target has sources then generate a unity build
        if not self.src:
            self.callback(subdir, None)
        else:
            f = unity(subdir, self.src)
            self.callback(subdir, f)



targets = {
    'ovmf': Target('ovmf', get_ovmf, 'download ovmf bios for qemu'),
    'bios': Target('bios', build_bios, 'build bezos targetting legacy bios', bios_src),
    'uefi': Target('uefi', build_uefi, 'build bezos targetting uefi', uefi_src)
}

if __name__ == "__main__":
    if not os.path.exists(builddir):
        os.mkdir(builddir)

    if 'help' in sys.argv or len(sys.argv) == 1:
        print('possible targets:')
        for name, target in targets.items():
            print(f'    - {name}: {target.desc}')
        exit(0)

    if 'release' in sys.argv:
        release = True

    for name, target in targets.items():
        if name in sys.argv:
            target.run()    
