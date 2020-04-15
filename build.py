#!/usr/bin/env python3

#TODO: rewrite the build script again

import subprocess
import os
import sys
import shutil

subprocess.call([
    'clang', '--target=x86_64-pc-win32-coff', '-fno-stack-protector', '-fshort-wchar', '-mno-red-zone', 
    '-Isubprojects',
    '-c', 'src/boot/uefi/uefi.c', '-o', 'uefi.o'
])
subprocess.call([
    'lld-link', '-subsystem:efi_application', '-nodefaultlib', '-dll', '-entry:efi_main', 'uefi.o', '-out:BOOTX64.EFI'
])

subprocess.call(['dd', 'if=/dev/zero', 'of=fat.img', 'bs=1k', 'count=1440'])
subprocess.call(['mformat', '-i', 'fat.img', '-f', '1440', '::'])
subprocess.call(['mmd', '-i', 'fat.img', '::/EFI'])
subprocess.call(['mmd', '-i', 'fat.img', '::/EFI/BOOT'])
subprocess.call(['mcopy', '-i', 'fat.img', 'BOOTX64.EFI', '::/EFI/BOOT'])

'''
def cc(): return 'clang'
def cxx(): return 'clang++'
def ld(target): return 'lld-link' if target == 'uefi' else 'ld.lld'

def cflags():
    pass

def cxxflags():
    pass

def ldflags():
    pass

cflags = [ '-ffreestanding', '-fno-stack-protector', '-fpic', '-fshort-wchar', '-MMD', '-mno-red-zone', '-std=c++17', '--target=x86_64-pc-win32-coff', '-Wall', '-Wextra' ]
cxxflags = [ 
    '-ffreestanding', 
    '-fno-stack-protector', 
    '-fpic', 
    '-fshort-wchar', 
    '-MMD', 
    '-mno-red-zone', 
    '-std=c++17', 
    '--target=x86_64-pc-win32-coff', 
    '-Wall', 
    '-Wextra' 
]

ldflags = [ '-subsystem:efi_application', '-entry:efi_main', '-nodefaultlib', '-dll' ]

def get_ovmf(path):
    subprocess.call(['wget', '--quiet', 'https://dl.bintray.com/no92/vineyard-binary/OVMF.fd', '-O', f'{path}/OVMF.fd'])
    exit(0)

def build_bios(path):
    pass

def build_uefi(path):
    uefi_src = [ 'src/boot/uefi/uefi.cpp' ]
    uefi_cxx = [ 
        '-ffreestanding', 
        '-fno-stack-protector', 
        '-fshort-wchar',
        '-MMD',
        '-mno-red-zone',
        '-std=c++17',
        '--target=x86_64-pc-win32-coff',
        '-Wall',
        '-Wextra',
        '-Werror'
    ]
    uefi_ld = [
        '-subsystem:efi_application',
        '-nodefaultlib',
        '-dll',
        '-entry:efi_main'
    ]

    subprocess.call(['clang++'] + uefi_cxx + uefi_src + ['-o', f'{path}/bezos.o'])
    #subprocess.call(['lld-link'] + uefi_ld + [f'{path}/bezos.o', f'-out:{path}/BOOTX64.EFI'])

    #image = f'{path}/uefi.img'

    # generate a bootable uefi image that can be put on a disk
    #subprocess.call(['dd', 'if=/dev/zero', f'of={image}', 'bs=1k', 'count=1440'])

    #subprocess.call(['mformat', '-i', image, '-f', '1440', '::'])
    #subprocess.call(['mmd', '-i', image, '::/EFI'])
    #subprocess.call(['mmd', '-i', image, '::/EFI/BOOT'])
    #subprocess.call(['mcopy', '-i', image, f'{path}/BOOTX64.EFI', '::/EFI/BOOT'])

    #if not os.path.exists(f'{path}/iso'):
    #    os.mkdir(f'{path}/iso')

    #shutil.copy(image, f'{path}/iso/bezos.img')

    #subprocess.call(['xorriso', '-as', 'mkisofs', '-R', '-f', '-e', f'{path}/iso/bezos.img', '-no-emul-boot', '-o', 'bezos.iso', 'iso'])



builddir = 'build'

class Target:
    def __init__(self, name, callback, desc = 'no description'):
        self.name = name
        self.callback = callback
        self.desc = desc

    def run(self):
        if not os.path.exists(f'{builddir}/{self.name}'):
            os.mkdir(f'{builddir}/{self.name}')

        self.callback(f'{builddir}/{self.name}')
        

targets = {
    'ovmf': Target('ovmf', get_ovmf, 'download ovmf bios for qemu'),
    'bios': Target('bios', build_bios, 'build bezos targetting legacy bios'),
    'uefi': Target('uefi', build_uefi, 'build bezos targetting uefi')
}

if __name__ == "__main__":
    if not os.path.exists(builddir):
        os.mkdir(builddir)

    if 'help' in sys.argv or len(sys.argv) == 1:
        print('possible targets:')
        for name, target in targets.items():
            print(f'    - {name}: {target.desc}')
        exit(0)

    for name, target in targets.items():
        if name in sys.argv:
            target.run()    

'''
'''
import subprocess
import os
import sys

asm = 'nasm'
asm_args = [ '-felf64' ]
asm_src = [ 'src/boot/bios/boot.asm' ]

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
    '-mcmodel=kernel',

    '-I.',
    '-Isrc',
    '-Isrc/kernel',
    '-m64',
    '-Wall',
    '-Wextra',
    '-Werror'
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

    if not os.path.exists('build'):
        os.mkdir('build')

    if 'ovmf' in sys.argv:
        if not os.path.exists('build/uefi'):
            os.mkdir('build/uefi')

        subprocess.call(['wget', '--quiet', 'https://dl.bintray.com/no92/vineyard-binary/OVMF.fd', '-O', 'build/uefi/OVMF.fd'])
        exit(0)


    if 'uefi' in sys.argv:
        # targetting uefi 64 bit
        outdir = 'build/uefi'
        cxx_args += [
            '--target=x86_64-pc-win32-coff',
            '-I/usr/include/efi',
            '-I/usr/include/efi/x86_64',
            '-I/usr/include/efi/protocol'
        ]

        ld_args += [
            '-nostdlib',
            '-fuse-ld=lld-link',
            '-Wl,-entry:efi_main',
            '-Wl,-subsystem:efi_application'
        ]
        cxx_src.append('src/boot/uefi/uefi.cpp')

        if not os.path.exists(outdir):
            os.mkdir(outdir)
    elif 'grub' in sys.argv:
        # targetting grub and stepping up to 64 bit
        pass
    elif 'grub2' in sys.argv:
        # targetting grub2 and stepping up to 64 bit
        pass
    elif 'qloader2' in sys.argv:
        # targetting qloader2
        pass
    else:
        # we default to rolling our own bootloader
        outdir = 'build/bios'
        cxx_args += [
            '-fno-pic',
            '-fno-pie'
        ]
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
'''