project('bezos', ['c', 'cpp'],
    default_options : [
        'cpp_std=c++17',
        'c_std=gnu99',
        'cpp_eh=none',
        'cpp_rtti=no',
        'warning_level=3'
    ],
    version : '0.0.1'
)

sources = []

args = [
    '-ffreestanding',
    '-nostdlib',
    '-mcmodel=kernel',
    '-mno-red-zone',
    '-march=native',
    '-fno-asynchronous-unwind-tables',
    '-fno-common'
]

ld_args = [
    '-ffreestanding',
    '-nostdlib',
    '-mcmodel=kernel',
    '-fno-use-linker-plugin',
    '-lgcc'
]

nasm = find_program('nasm')
nasm_gen = generator(nasm,
    output : '@BASENAME@.o',
    arguments : [ '-felf64', '@INPUT@', '-o', '@OUTPUT@']
)

linker = ''
asm = []
incs = []

subdir('kernel')

ld_args += '-T' + linker

asm_files = nasm_gen.process(asm)

executable('bezos.elf', asm_files, sources,
    include_directories : incs,
    cpp_args : args,
    c_args : args,
    link_args : ld_args
)