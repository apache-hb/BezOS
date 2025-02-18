project('system', 'cpp',
    default_options : [
        'cpp_rtti=false',
        'cpp_eh=none',
    ]
)

flanterm = dependency('flanterm')
shared = subproject('shared')
sysapi = shared.get_variable('bezos_dep')

args = [
    '-e', 'ClientStart',
    '-z', 'noexecstack',
]

src = [
    'init/init.cpp',
]

executable('init.elf', src,
    link_args : args,
    install : true,
    dependencies : [ sysapi, flanterm ],
)

src = [
    'shell/shell.cpp',
]

executable('shell.elf', src,
    link_args : args,
    install : true,
    dependencies : sysapi,
)
