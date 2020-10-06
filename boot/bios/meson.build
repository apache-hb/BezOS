assemble = generator(find_program('nasm'),
    output : '@BASENAME@.o',
    arguments : [ '-felf64', '@INPUT@', '-o', '@OUTPUT@' ]
)

ldscript = meson.current_source_dir()/'link.ld'

kernel = executable('bezos.bin', [ 'boot.cpp', assemble.process('boot.asm') ],
    dependencies : kernel,
    link_args : [ '-Wl,-T' + ldscript ],
    link_depends : ldscript
)
