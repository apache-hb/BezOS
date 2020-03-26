project('bezos', [ 'c', 'cpp' ],
    default_options : [
        'c_std=gnu99',
        'cpp_std=c++17',
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

kernel_elf = executable('bezos.elf', sources,
    include_directories : incs,
    cpp_args : args + [ '-m64' ],
    link_args : links + [ '-Wl,-T' + linker ],
    link_depends : linker
)

objcopy = find_program('objcopy')

kernel = custom_target('flatten',
    input : kernel_elf,
    output : 'bezos.bin',
    command : [ objcopy, '@INPUT@', '@OUTPUT@', '-j', '.kernel', '-O', 'binary' ],
    build_by_default : true
)