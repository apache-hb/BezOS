project('bezos', ['c', 'cpp'],
    default_options : [
        'cpp_std=c++17',
        'c_std=gnu99',
        'cpp_eh=none',
        'cpp_rtti=false'
    ],
    version : '0.0.1',
    license : 'Apache2'
)

sources = []

args = [
    '-ffreestanding', 
    '-mno-red-zone', 
    '-mcmodel=kernel', 
    '-fno-unwind-tables', 
    '-fno-asynchronous-unwind-tables', 
    '-fomit-frame-pointer',
    '-fno-PIC',
    '-march=native'
]

nasm = find_program('nasm')
nasm_gen = generator(nasm,
    output : '@BASENAME@.o',
    arguments : [ '-felf64', '@INPUT@', '-o', '@OUTPUT@']
)

asm = []

subdir('arch'/meson.get_cross_property('arch'))
subdir('kernel')

asm_out = nasm_gen.process(asm)

ld_args = [
    '-ffreestanding', 
    '-nostdlib', 
    '-lgcc', 
    '-no-pie', 
    '-fno-use-linker-plugin',
    '-T' + linker
]

kernel = executable('kernel.bin', asm_out, sources,
    c_args : args,
    cpp_args : args,
    link_args : ld_args
)

copy = find_program(meson.get_cross_property('objcopy'))
copy_gen = generator(copy,
    output : '@BASENAME@.bin',
    arguments : [ '-O', 'elf32-i386', '@INPUT@', '@OUTPUT@' ]
)

kern32 = copy_gen.process(kernel)

kernel32 = executable('kernel32', kern32)