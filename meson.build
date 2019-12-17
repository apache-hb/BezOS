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
    '-fno-asynchronous-unwind-tables'
]

ld_args = [
    '-nostdlib',
    '-ffreestanding',
    '-mcmodel=kernel',
    '-fno-use-linker-plugin',
    '-lgcc'
]

nasm = find_program('nasm')

linker = ''
asm = []

subdir('kernel')

ld_args += '-T' + linker

compiled_asm = []

foreach a : asm

endforeach

executable('bezos.elf', sources,
    cpp_args : args,
    c_args : args,
    link_args : ld_args
)