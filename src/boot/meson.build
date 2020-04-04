link = meson.current_source_dir()/'link.ld'

sources += asm.process([
    'real.asm',
    'prot.asm',
    'long.asm'
])

libprot = static_library('prot', 'prot.cpp',
    cpp_args : args + [ '-m32' ],
    include_directories : incs
)

prot = custom_target('prot64', 
    input : libprot,
    output : 'prot.a',
    command : [ objcopy, '@INPUT@', '@OUTPUT@', '-O', 'elf64-x86-64' ]
)