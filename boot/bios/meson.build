ldscript = meson.current_source_dir()/'link.ld'

kscript = meson.current_source_dir()/'klink.ld'

kernel = executable('kernel.bin', src,
    include_directories : inc,
    link_args : [ '-Wl,-T' + kscript, '-mcmodel=kernel' ],
    link_depends : kscript
)

assemble = generator(find_program('nasm'),
    output : '@BASENAME@.o',
    depends : kernel,
    arguments : [ '-felf64', '@INPUT@', '-o', '@OUTPUT@', '-i', meson.current_build_dir() + '/' ]
)

bezos = executable('bezos.bin', [ 'boot.cpp', assemble.process('boot.asm') ],
    link_args : [ '-Wl,-T' + ldscript ],
    link_depends : [ ldscript, kernel ],
    include_directories : inc
)
