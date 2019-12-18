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
    '-T' + linker
]

executable('kernel.bin', asm_out, sources,
    link_args : ld_args
)