project('bezos', ['c', 'cpp'],
    default_options : [
        'cpp_rtti=false',
        'cpp_eh=none',
        'cpp_std=c++17',
        'c_std=c99',
        'warning_level=3',
        'werror=true',
        'debug=false',
        'optimization=3'
    ]
)

assemble = generator(
    find_program('nasm'),
    output : '@BASENAME@.o',
    arguments : [ '-felf64', '@INPUT@', '-o', '@OUTPUT@' ]
)

arch = 'src/arch'/meson.get_cross_property('arch')

sources = []
links = []
args = []
incs = []

incs += 'src'

subdir('src/kernel')
subdir(arch)

links += '-T' + linker

kernel = executable('bezos.bin', # name
    sources, # source files 
    include_directories : incs,
    cpp_args : args,
    link_args : links,
    link_depends : linker
)
