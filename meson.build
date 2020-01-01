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
deps = []
args = []
incs = []

if arch == 'src/arch/x86-64'
    args += '-DTARGET_X86=1'
endif

if arch == 'src/arch/armv8'
    args += '-DTARGET_ARM=1'
endif

incs += include_directories('src')

subdir('src/kernel')
subdir(arch)

links += '-Wl,-T' + linker

if meson.get_compiler('cpp').get_id() != 'clang'
    error('We only support clang')
endif

kernel = executable('bezos.bin', # name
    sources, # source files 
    include_directories : incs,
    cpp_args : args,
    link_args : links,
    link_with : deps
)
