project('bezos', 'c',
    default_options : [
        'c_std=gnu99',
        'debug=false',
        'optimization=3',
        'b_pie=false',
        'b_staticpic=false'
    ]
)

sources = []
args = []
links = []
incs = [ include_directories('src') ]

assemble = generator(
    find_program('nasm'),
    output : '@BASENAME@.o',
    arguments : [ '-felf64', '@INPUT@', '-o', '@OUTPUT@' ]
)

subdir('src/boot')
subdir('src/kernel')

kernel = executable('bezos.bin', sources,
    include_directories : incs,
    cpp_args : args + [ '-m64' ],
    link_args : links + [ '-Wl,-T' + linker ],
    link_depends : linker
)