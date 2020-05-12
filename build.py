#!/usr/bin/env python3

from subprocess import call
import os

if __name__ == "__main__":
    try:
        # create output dir
        os.mkdir('build')
    except FileExistsError:
        pass

    # build bootloader
    call(['nasm', '-felf64', 'src/boot.asm', '-o', 'build/boot.o'])

    call(['ld', 'build/boot.o', '-Tsrc/link.ld', '-o', 'build/bezos'])
