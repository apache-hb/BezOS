project('bezos', [ 'c', 'cpp' ],
    default_options : [
        'c_std=c11',
        'cpp_std=c++17',
        'debug=false',
        #'layout=flat',
        'warning_level=3',
        'werror=true',
        'unity=on',
        'b_lto=true',
        'b_pie=false',
        'b_staticpic=false'
    ]
)

sources = []
args = []
links = []
incs = [ include_directories('src') ]

asm = generator(
    find_program('nasm'),
    output : '@BASENAME@.o',
    arguments : [ '-felf64', '@INPUT@', '-o', '@OUTPUT@' ]
)

subdir('src/boot')
subdir('src/kernel')

elf = executable('bezos.elf', sources,
    include_directories : incs,
    cpp_args : args + [ '-m64' ],
    link_args : links + [ '-Wl,-T' + link ],
    link_depends : link
)

objcopy = find_program('objcopy')

kernel = custom_target('flatten',
    input : elf,
    output : 'bezos.bin',
    command : [ objcopy, '@INPUT@', '@OUTPUT@', '-R', '.ignore', '-O', 'binary' ],
    build_by_default : true
)