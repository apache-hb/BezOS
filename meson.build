project('bezos', 'c',
    default_options : [
        'c_std=c99',
        'werror=true',
        'warning_level=3',
        'b_pie=false',
        'b_staticpic=false',
        'debug=false'
    ]
)

nasm = find_program('nasm')
assemble = generator(nasm,
    output : '@BASENAME@.o',
    arguments : [ '-felf64', '@INPUT@', '-o', '@OUTPUT@' ]
)

sources = [ 'src/main.c' ]

subdir('src/boot')

elf = executable('bezos', sources,
    link_args : [ '-Wl,-T' + ldscript ],
    link_depends : ldscript
)

kernel = custom_target('kernel',
    output : 'bezos.bin',
    input : elf,
    build_by_default : true,
    command : [ 'objcopy', '-R .ignore', '@INPUT@', '-O', 'binary', '@OUTPUT@' ]
)