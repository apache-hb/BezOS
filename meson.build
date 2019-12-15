project('bezos', ['c', 'cpp'],
    default_options : [
        'cpp_std=c++17',
        'c_std=gnu99',
        'cpp_eh=none',
        'cpp_rtti=false'
    ],
    version : '0.0.1'
)

sources = []
incs = []

links = []

args = []

subdir('kernel')

kernel = executable('bezos.elf', sources,
    include_directories : incs,
    cpp_args : args,
    link_with : links
)