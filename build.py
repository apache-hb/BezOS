#!/usr/bin/env python3

import subprocess
import os
import sys

asm = 'nasm'
asm_args = [ '-felf64' ]
asm_src = [ 'src/bios/boot.asm' ]

cxx = 'clang++'
cxx_args = [
    '-ffreestanding',
    '-fno-exceptions',
    '-fno-rtti',
    '-fno-addrsig',
    '-fno-builtin',
    '-fno-common',
    '-nostdlib',
    '-static',
    '-mno-sse',
    '-mno-sse2',
    '-mno-mmx',
    '-ffast-math',
    '-fdelete-null-pointer-checks',
    '-fno-threadsafe-statics',
    '-fno-stack-protector',
    '-mno-red-zone',
    '-std=c++17',
    '-fno-pic',
    '-pipe',
    '-fshort-wchar',

    '-I.',
    '-Isrc',
    '-Isrc/kernel',
    '-m64',
    '-Wall',
    '-Wextra',
    '-Werror',
    '-fno-pic'
]
cxx_src = [ 
    'src/kernel/kmain.cpp',
    'src/kernel/mm/mm.cpp'
]

ld = 'clang'
ld_args = [
    '-ffreestanding', 
    '-nostartfiles', 
    '-nostdlib'
]

objcopy = 'objcopy'

def cc_asm(path, output):
    subprocess.call([asm] + asm_args + [path, '-o', f'{output}.o'])
    return f'{output}.o'

def cc_cxx(files, output):
    subprocess.call([cxx] + cxx_args + ['-o', f'{output}.o', '-c', files])
    return f'{output}.o'

def cc_link(objects, output):
    subprocess.call([ld] + ld_args + ['-o', output] + objects)
    return output

def image(obj, output):
    subprocess.call([objcopy, obj, output, '-R', '.ignore', '-O', 'binary'])
    return output

if __name__ == "__main__":
    objects = []
    
    if 'release' in sys.argv:
        cxx_args.append('-O3')

    if 'uefi' in sys.argv:
        outdir = 'uefi'
        cxx_args += [
            '--target=x86_64-unknown-windows',
            '-I/usr/include/efi',
            '-I/usr/include/efi/x86_64',
            '-I/usr/include/efi/protocol'
        ]

        ld_args += [
            '--target=x86_64-unknown-windows',
            '-nostdlib',
            '-fuse-ld=lld-link',
            '-Wl,-entry:efi_main',
            '-Wl,-subsystem:efi_application'
        ]
        cxx_src.append('src/uefi/uefi.cpp')

        if not os.path.exists(outdir):
            os.mkdir(outdir)
    else:
        outdir = 'bios'
        ld_args.append('-Wl,-Tsrc/link.ld')
        
        if not os.path.exists(outdir):
            os.mkdir(outdir)

        for src in asm_src:
            objects.append(cc_asm(src, f'{outdir}/{".".join(src.split("/"))}'))

    # generate unity build file
    with open(f'{outdir}/unity.cpp', 'w+') as unity:
        for src in cxx_src:
            unity.write(f'#include "{src}"\n')

    out = cc_cxx(f'{outdir}/unity.cpp', f'{outdir}/unity')


    if 'uefi' in sys.argv:
        cc_link(objects + [out], f'{outdir}/BOOTX64.EFI')
    else:
        elf = cc_link(objects + [out], f'{outdir}/bezos.elf')
        image(elf, f'{outdir}/bezos.bin')
