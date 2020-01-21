project('bezos', 'c',
    default_options : [
        'c_std=c11',
        'warning_level=3',
        'werror=true',
        'debug=false',
        'optimization=3',
        'b_pie=false',
        'b_staticpic=false'
    ]
)

sources = []
sources32 = []
args = []
links = []
incs = [ include_directories('src') ]

assemble = generator(
    find_program('nasm'),
    output : '@BASENAME@.o',
    arguments : [ '-felf64', '@INPUT@', '-o', '@OUTPUT@' ]
)

subdir('src/arch'/meson.get_cross_property('arch'))

subdir('src/kernel')

kernel32 = static_library('32', sources32,
    include_directories : incs,
    cpp_args : args + [ '-m32', '-fno-pic' ]
)

if host_machine.system() == 'darwin'
    objcopy = find_program('objcopy')
else
    objcopy = find_program('llvm-objcopy')
endif

copy = custom_target('lib32as64',
    input : kernel32,
    output : '32as64.o',
    command : [ objcopy, '-O', 'elf64-x86-64', '@INPUT@', '@OUTPUT@' ]
)

sources += copy

kernel = executable('bezos.bin', sources,
    include_directories : incs,
    cpp_args : args + [ '-m64' ],
    link_args : links + [ '-Wl,-T' + linker ],
    link_depends : linker
)